#pragma once
// Screen printing on the QR204 thermal printer (the ESP32-C3 bridge is a separate project).
//
// The printer hangs off an ESP32-C3 bridge that forwards raw TCP (port 9100) to its UART, so this side only
// produces ESC/POS bytes. On a print request the display lambda draws the current page a second time into
// Canvas, a 1-bit copy of the 240x320 screen; start() turns it into ESC/POS (header text + GS v 0 raster,
// 1x or 1.5x, centered in the printer's 384 dots) and hands it to a FreeRTOS task that sends it, so the loop
// never waits for the network.
//
// Used from the YAML: the display lambda (canvas, set_ink_threshold, Options, start; the options come from the
// "Print ..." entities in the web UI), the print_screen script (busy) and a 500 ms interval (state,
// status_text).

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esphome/components/display/display.h"
#include "esphome/core/log.h"

namespace thermalprint {

static const char *const TAG = "thermalprint";

// Screen as the display lambda sees it (portrait, no rotation).
static const int SCREEN_W = 240;
static const int SCREEN_H = 320;
// QR204: 58 mm paper, 384 dots per line at 203 dpi.
static const int PRINTER_DOTS = 384;
static const int ROW_BYTES = PRINTER_DOTS / 8;
// Largest GS v 0 block in bytes. The printer holds back a block until it has all of it only up to ~1 KB; a larger
// one starts printing early and then waits for data at 9600 baud, and every stop leaves a white line in dark areas
// (measured: 1128-byte blocks got a line, 564-byte blocks printed clean). 12 rows at 1.5x, 14 at 1x.
static const int BAND_BYTES = 576;
// A pixel prints black when its brightest channel reaches the ink threshold (web UI "Print Ink Threshold").
// The UI is dark, so the black background becomes paper and everything drawn on it becomes ink; with the
// default, grey (0xAA) and pure blue (0x0000FF) stay visible and the faint anti-aliasing fringe of the 4 bpp
// fonts is dropped. Lower keeps more of the fringe (bolder text), higher gives thinner text.
static const uint8_t DEFAULT_INK_THRESHOLD = 0x40;
// NUL bytes after the header text. While the printer prints the two header lines, image data piles up; it then
// prints that backlog at full speed, has to stop abruptly when it runs out, and leaves a white line ~5 mm into
// the image. With the spacer the backlog is harmless NULs and the image arrives while the printer waits for it
// (measured: 1000 bytes still left the line, 2500 didn't). Costs ~3 s per print with the header.
static const int HEADER_SPACER = 3000;

// Per-print settings from the web UI.
struct Options {
  bool large = true;       // 1.5x (360 x 480 dots, 45 x 60 mm, ~24 s at 9600 baud) instead of 1x (240 x 320
                           // dots, 30 x 40 mm, ~13 s)
  bool header = true;      // box name and date above the image
  uint8_t feed_lines = 5;  // ESC d after the image (plus one line feed), so the end clears the tear bar
};

enum State { IDLE, SENDING, DONE, FAILED };

inline std::atomic<int> state{IDLE};
inline std::string last_error;  // written by the send task before state becomes FAILED

// 1-bit off-screen copy of the display. The display lambda draws into it like into the real screen.
class Canvas : public esphome::display::Display {
 public:
  bool ready() const { return this->bits_ != nullptr; }
  void clear_bits() {
    if (this->bits_ == nullptr) {
      // Kept for the whole uptime once allocated; PSRAM first, internal RAM as fallback.
      this->bits_ = (uint8_t *) heap_caps_malloc(BYTES, MALLOC_CAP_SPIRAM);
      if (this->bits_ == nullptr)
        this->bits_ = (uint8_t *) malloc(BYTES);
      if (this->bits_ == nullptr)
        return;
    }
    memset(this->bits_, 0, BYTES);
  }
  bool get(int x, int y) const { return this->bits_[y * (SCREEN_W / 8) + x / 8] & (0x80 >> (x % 8)); }
  void set_ink_threshold(uint8_t threshold) { this->ink_threshold_ = threshold; }

  void draw_pixel_at(int x, int y, esphome::Color color) override {
    if (this->bits_ == nullptr || x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H)
      return;
    if (!this->get_clipping().inside(x, y))
      return;
    uint8_t &byte = this->bits_[y * (SCREEN_W / 8) + x / 8];
    uint8_t mask = 0x80 >> (x % 8);
    if (std::max({color.r, color.g, color.b}) >= this->ink_threshold_) {
      byte |= mask;
    } else {
      byte &= ~mask;
    }
  }
  esphome::display::DisplayType get_display_type() override { return esphome::display::DISPLAY_TYPE_BINARY; }
  void update() override {}

 protected:
  static const size_t BYTES = SCREEN_W / 8 * SCREEN_H;
  int get_width_internal() override { return SCREEN_W; }
  int get_height_internal() override { return SCREEN_H; }
  uint8_t *bits_ = nullptr;
  uint8_t ink_threshold_ = DEFAULT_INK_THRESHOLD;
};

inline Canvas &canvas() {
  static Canvas instance;
  return instance;
}

inline bool busy() { return state == SENDING; }

inline const char *status_text() {
  switch (state.load()) {
    case SENDING:
      return "Sending";
    case DONE:
      return "Done";
    case FAILED:
      return last_error.c_str();
    default:
      return "Idle";
  }
}

// Growing byte buffer in PSRAM for one job (~24 KB).
struct Buffer {
  uint8_t *data = nullptr;
  size_t len = 0;
  size_t cap = 0;

  bool reserve(size_t n) {
    data = (uint8_t *) heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
    if (data == nullptr)
      data = (uint8_t *) malloc(n);
    cap = data != nullptr ? n : 0;
    return data != nullptr;
  }
  void put(const void *src, size_t n) {
    if (len + n > cap)
      return;  // sized up front; never hit
    memcpy(data + len, src, n);
    len += n;
  }
  void put(std::initializer_list<uint8_t> bytes) {
    for (uint8_t b : bytes)
      put(&b, 1);
  }
  // UTF-8 to what the printer can print. Its firmware stays in GB18030 mode (FS . and ESC t are ignored) and its
  // font has no ä, ö, ü or ß: umlauts become ae/oe/ue/ss, ° and µ their GB2312 codes, ² and ³ plain digits,
  // anything else '?'. Never longer than the input. Same mapping as to_printer() in the bridge's printer_bridge.h.
  void text(const std::string &s) {
    static const struct {
      uint32_t code;
      const char *text;
    } MAP[] = {{0xE4, "ae"}, {0xF6, "oe"}, {0xFC, "ue"}, {0xC4, "Ae"}, {0xD6, "Oe"}, {0xDC, "Ue"}, {0xDF, "ss"},
               {0xB0, "\xA1\xE3"}, {0xB5, "\xA6\xCC"}, {0x3BC, "\xA6\xCC"}, {0xB2, "2"}, {0xB3, "3"}};
    for (size_t i = 0; i < s.size();) {
      uint8_t c = s[i];
      int n = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 1;
      uint32_t code = c < 0x80 ? c : n == 1 ? 0xFFFD : c & (0x7F >> n);  // 0xFFFD: invalid byte, printed as '?'
      for (int k = 1; k < n && i + k < s.size(); k++)
        code = (code << 6) | (s[i + k] & 0x3F);
      i += n;
      if (code < 0x80) {
        put({(uint8_t) code});
        continue;
      }
      const char *text = "?";
      for (const auto &m : MAP) {
        if (m.code == code)
          text = m.text;
      }
      put(text, strlen(text));
    }
  }
};

struct Job {
  Buffer buf;
  std::string host;
  uint16_t port;
};

inline std::string send_job(const Job &job) {
  addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *res = nullptr;
  char port[8];
  snprintf(port, sizeof(port), "%u", job.port);
  if (lwip_getaddrinfo(job.host.c_str(), port, &hints, &res) != 0 || res == nullptr)
    return "Error: cannot resolve " + job.host;

  char addr[16];
  inet_ntop(AF_INET, &((sockaddr_in *) res->ai_addr)->sin_addr, addr, sizeof(addr));
  ESP_LOGD(TAG, "Resolved %s to %s", job.host.c_str(), addr);
  int fd = lwip_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    lwip_freeaddrinfo(res);
    return "Error: no socket";
  }
  // lwip_* calls directly: ESPHome builds without VFS select support, so plain select() would not see the
  // socket. Non-blocking connect with a 5 s limit, then blocking sends with a 30 s limit each.
  lwip_fcntl(fd, F_SETFL, lwip_fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  int rc = lwip_connect(fd, res->ai_addr, res->ai_addrlen);
  lwip_freeaddrinfo(res);
  if (rc != 0 && errno != EINPROGRESS) {
    lwip_close(fd);
    return "Error: connect failed";
  }
  if (rc != 0) {
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    timeval tv = {5, 0};
    int err = 0;
    socklen_t len = sizeof(err);
    if (lwip_select(fd + 1, nullptr, &wfds, nullptr, &tv) != 1 ||
        lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0) {
      lwip_close(fd);
      return "Error: printer bridge not reachable";
    }
  }
  ESP_LOGD(TAG, "Connected to %s:%u", addr, job.port);
  lwip_fcntl(fd, F_SETFL, lwip_fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
  timeval send_timeout = {30, 0};
  lwip_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

  size_t sent = 0;
  while (sent < job.buf.len) {
    ssize_t n = lwip_send(fd, job.buf.data + sent, job.buf.len - sent, 0);
    if (n <= 0) {
      lwip_close(fd);
      return "Error: send failed after " + std::to_string(sent) + " bytes";
    }
    sent += n;
  }
  // Half-close, then wait until the bridge has read everything and closes its side (it does after the last
  // byte), so "Sent" means the bridge has the whole job. Bytes the printer sends back are read and dropped.
  lwip_shutdown(fd, SHUT_WR);
  timeval recv_timeout = {60, 0};
  lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
  char discard[64];
  ssize_t n;
  while ((n = lwip_recv(fd, discard, sizeof(discard), 0)) > 0) {
  }
  int err = n < 0 ? errno : 0;
  lwip_close(fd);
  if (n < 0)
    return err == EAGAIN || err == EWOULDBLOCK ? "Error: bridge did not confirm the job"
                                               : "Error: bridge dropped the job";
  return "";
}

inline void send_task(void *arg) {
  Job *job = static_cast<Job *>(arg);
  std::string err = send_job(*job);
  ESP_LOGI(TAG, "%s (%u bytes to %s:%u)", err.empty() ? "Sent" : err.c_str(), (unsigned) job->buf.len,
           job->host.c_str(), job->port);
  heap_caps_free(job->buf.data);
  delete job;
  if (err.empty()) {
    state = DONE;
  } else {
    last_error = err;
    state = FAILED;
  }
  vTaskDelete(nullptr);
}

// Builds the job from the canvas (header lines above the screen image) and starts sending it.
inline bool start(const std::string &host, uint16_t port, const std::string &title, const std::string &subtitle,
                  const Options &opt) {
  if (busy()) {
    ESP_LOGW(TAG, "Previous print still sending");
    return false;
  }
  Canvas &c = canvas();
  if (!c.ready()) {
    last_error = "Error: out of memory";
    state = FAILED;
    return false;
  }

  Job *job = new Job();
  job->host = host;
  job->port = port;
  // Nearest-neighbour scaling by num/den, centered; rows end at the image's right edge (the printer leaves the
  // rest of the line blank), which saves transfer time at 1x.
  const int num = opt.large ? 3 : 1, den = opt.large ? 2 : 1;
  const int out_w = SCREEN_W * num / den, out_h = SCREEN_H * num / den;
  const int left = (PRINTER_DOTS - out_w) / 2;
  const int row_bytes = (left + out_w + 7) / 8;
  const int band_rows = BAND_BYTES / row_bytes;
  const int bands = (out_h + band_rows - 1) / band_rows;
  if (!job->buf.reserve(256 + HEADER_SPACER + title.size() + subtitle.size() +
                        bands * (8 + row_bytes * band_rows))) {
    delete job;
    last_error = "Error: out of memory";
    state = FAILED;
    return false;
  }
  Buffer &b = job->buf;

  b.put({0x1B, '@'});         // ESC @: initialize
  if (opt.header) {
    b.put({0x1B, 'a', 1});    // ESC a 1: centered
    b.put({0x1B, 'E', 1});    // ESC E 1: bold
    b.text(title);
    b.put({'\n', 0x1B, 'E', 0});
    b.text(subtitle);
    b.put({'\n', 0x1B, 'a', 0});
    b.put({0x1B, 'J', 8});    // ESC J 8: 1 mm gap before the image
    for (int i = 0; i < HEADER_SPACER; i++)
      b.put({0});
  }

  uint8_t row[ROW_BYTES];
  for (int band_y = 0; band_y < out_h; band_y += band_rows) {
    int rows = std::min(band_rows, out_h - band_y);
    // GS v 0 m xL xH yL yH: raster image, width in bytes, height in dots.
    b.put({0x1D, 'v', '0', 0, (uint8_t) row_bytes, 0, (uint8_t) rows, 0});
    for (int y = band_y; y < band_y + rows; y++) {
      memset(row, 0, row_bytes);
      int src_y = y * den / num;
      for (int x = 0; x < out_w; x++) {
        if (c.get(x * den / num, src_y)) {
          int dot = left + x;
          row[dot / 8] |= 0x80 >> (dot % 8);
        }
      }
      b.put(row, row_bytes);
    }
  }
  // Line feed first: this firmware ignores ESC d right after a raster image (measured on this printer).
  b.put({'\n', 0x1B, 'd', opt.feed_lines});  // ESC d: feed past the tear bar

  state = SENDING;
  if (xTaskCreate(send_task, "thermalprint", 4096, job, tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
    heap_caps_free(b.data);
    delete job;
    last_error = "Error: cannot start send task";
    state = FAILED;
    return false;
  }
  ESP_LOGI(TAG, "Printing screen (%u bytes) to %s:%u", (unsigned) b.len, host.c_str(), port);
  return true;
}

}  // namespace thermalprint
