#pragma once
// SD card logging for the sensor box.
//
// The card slot sits on the MSP2807 display module (header J4), wired to the mainboard's GPIO header on
// its own SPI bus: SPI3_HOST, which this helper initializes. ESPHome's spi component only has the
// display's bus and gives it SPI2_HOST, so the two never share lines or locks. The card is mounted via
// ESP-IDF SDSPI + FATFS; a failed mount or unmount removes only the SD device (sdspi_host_remove_device),
// the bus stays initialized for the next attempt.
//
// All card access runs in its own FreeRTOS task ("sdlog"), fed by a small queue. A card that stops
// answering makes ESP-IDF wait up to 5 s per write (SDMMC_WRITE_CMD_TIMEOUT_MS), which in ESPHome's loop
// task tripped the 5 s task watchdog (reset on 2026-09-28 ~22:23). The loop task only queues requests and
// reads the status; the sdlog task isn't watched by the task watchdog.
//
// Used from the YAML: on_boot (begin), a 60 s interval (Row + log_row), template entities (get_status,
// free_mb) and buttons (request_mount, request_unmount).

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include "esphome/core/log.h"

namespace sdlog {

static const char *const TAG = "sdlog";
static const char *const MOUNT_POINT = "/sdcard";
static const spi_host_device_t SD_SPI_HOST = SPI3_HOST;
static const int QUEUE_LENGTH = 4;  // rows are queued once a minute; 4 cover a few minutes of a busy card
static const uint32_t TASK_STACK = 6144;
static const int FREE_REFRESH_ROWS = 10;  // re-read the free space every 10 rows written

// One CSV row in a fixed buffer; NaN values become empty fields.
struct Row {
  char buf[512];
  size_t len = 0;

  void add(const char *fmt, ...) {
    if (len >= sizeof(buf))
      return;
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    va_end(args);
    if (n > 0)
      len = std::min(sizeof(buf), len + (size_t) n);
  }
  void num(float v, int decimals) {
    if (std::isnan(v)) {
      add(",");
    } else {
      add(",%.*f", decimals, v);
    }
  }
  void text(const std::string &s) { add(",%s", s.c_str()); }
};

enum class MsgType : uint8_t { MOUNT, UNMOUNT, ROW };

struct Msg {
  MsgType type;
  char date[16];  // yyyymmdd; same size as the YAML's buffer
  const char *header;  // points to a static string in the YAML lambda
  Row row;
};

// Shared between the loop task and the sdlog task.
inline std::atomic<bool> mounted{false};
inline std::atomic<float> free_cached{NAN};
inline std::mutex status_mutex;
inline std::string status = "Not mounted";
inline QueueHandle_t queue = nullptr;

// Owned by the sdlog task after begin().
inline sdmmc_card_t *card = nullptr;
inline int clk_pin = -1, mosi_pin = -1, miso_pin = -1, cs_pin = -1;
inline bool bus_ready = false;
inline int rows_since_free = 0;

inline void set_status(const std::string &s) {
  std::lock_guard<std::mutex> lock(status_mutex);
  status = s;
}

inline std::string get_status() {
  std::lock_guard<std::mutex> lock(status_mutex);
  return status;
}

inline bool is_mounted() { return mounted.load(); }

inline float free_mb() { return free_cached.load(); }

// ---- sdlog task only below, up to the public request functions ----

// Defined idle state for CS while no card is mounted.
inline void hold_cs_high(int cs_gpio) {
  gpio_reset_pin((gpio_num_t) cs_gpio);
  gpio_set_direction((gpio_num_t) cs_gpio, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t) cs_gpio, 1);
}

inline bool init_bus() {
  hold_cs_high(cs_pin);
  spi_bus_config_t bus = {};
  bus.sclk_io_num = clk_pin;
  bus.mosi_io_num = mosi_pin;
  bus.miso_io_num = miso_pin;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = 4000;
  esp_err_t err = spi_bus_initialize(SD_SPI_HOST, &bus, SPI_DMA_CH_AUTO);
  if (err != ESP_OK) {
    set_status(std::string("SPI bus init failed: ") + esp_err_to_name(err));
    ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
    return false;
  }
  // The module has no pull-up on SD_MISO; without a card this also makes the init fail fast (0xFF).
  gpio_set_pull_mode((gpio_num_t) miso_pin, GPIO_PULLUP_ONLY);
  bus_ready = true;
  return true;
}

// Free space from the FAT32 FSINFO sector, read raw. esp_vfs_fat_info() (f_getfree) scans the whole FAT
// when FatFs doesn't know the count (FSINFO unreadable at mount): ~15,000 sectors on these cards, minutes at
// 1 MHz. FatFs keeps FSINFO current on every fclose when it was readable at mount; otherwise this shows
// the count from the last good mount.
inline float read_free_mb() {
  if (card == nullptr)
    return NAN;
  alignas(4) static uint8_t buf[512];
  auto read_sector = [](uint32_t sector) { return sdmmc_read_sectors(card, buf, sector, 1) == ESP_OK; };
  auto u16 = [](const uint8_t *p) { return (uint32_t) p[0] | (uint32_t) p[1] << 8; };
  auto u32 = [](const uint8_t *p) {
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
  };
  // Sector 0 is the MBR (first partition's start at 454), or the boot sector itself without partitions.
  if (!read_sector(0))
    return NAN;
  uint32_t boot = memcmp(buf + 82, "FAT32", 5) == 0 ? 0 : u32(buf + 454);
  if (boot != 0 && !read_sector(boot))
    return NAN;
  if (memcmp(buf + 82, "FAT32", 5) != 0)
    return NAN;
  uint32_t bytes_per_sector = u16(buf + 11), sectors_per_cluster = buf[13], fsinfo = u16(buf + 48);
  if (!read_sector(boot + fsinfo) || u32(buf) != 0x41615252 || u32(buf + 484) != 0x61417272)
    return NAN;
  uint32_t free_clusters = u32(buf + 488);
  if (free_clusters == 0xFFFFFFFF)
    return NAN;
  return free_clusters * (float) (sectors_per_cluster * bytes_per_sector) / (1024.0f * 1024.0f);
}

inline bool do_mount() {
  if (card != nullptr)
    return true;
  if (!bus_ready && !init_bus())
    return false;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI_HOST;
  // 1 kOhm series resistors on SD_CS/SD_MOSI/SD_CLK on the module plus Dupont wires round the edges:
  // 10 MHz gave intermittent ESP_ERR_INVALID_RESPONSE (0x108) on reads and writes, 4 MHz still on
  // writes (512-byte data blocks on MOSI). The log needs a few sectors per minute, so speed doesn't matter.
  host.max_freq_khz = 1000;

  sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot.host_id = SD_SPI_HOST;
  slot.gpio_cs = (gpio_num_t) cs_pin;

  esp_vfs_fat_mount_config_t cfg = {};  // never format: a failed mount must not wipe the card
  cfg.format_if_mount_failed = false;
  cfg.max_files = 2;

  esp_err_t err = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot, &cfg, &card);
  if (err != ESP_OK) {
    card = nullptr;
    set_status(std::string("Mount failed: ") + esp_err_to_name(err));
    ESP_LOGW(TAG, "Mount failed: %s", esp_err_to_name(err));
    hold_cs_high(cs_pin);
    return false;
  }
  mounted = true;
  set_status("Mounted");
  uint64_t size_mb = (uint64_t) card->csd.capacity * card->csd.sector_size / (1024 * 1024);
  ESP_LOGI(TAG, "Mounted card '%s', %llu MB", card->cid.name, (unsigned long long) size_mb);
  free_cached = read_free_mb();
  rows_since_free = 0;
  return true;
}

inline void do_unmount(const char *reason) {
  if (card != nullptr) {
    mounted = false;
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    card = nullptr;
    ESP_LOGI(TAG, "Unmounted");
  }
  free_cached = NAN;
  set_status(reason);
  if (cs_pin >= 0)
    hold_cs_high(cs_pin);
}

inline bool write_row(const char *path, const char *header, const Row &row) {
  FILE *f = fopen(path, "a");
  if (f == nullptr) {
    ESP_LOGW(TAG, "Cannot open %s", path);
    return false;
  }
  bool ok = fseek(f, 0, SEEK_END) == 0;
  if (ok && ftell(f) == 0)
    ok = fprintf(f, "%s\n", header) > 0;
  ok = ok && fwrite(row.buf, 1, row.len, f) == row.len && fputc('\n', f) != EOF;
  ok = (fclose(f) == 0) && ok;
  if (!ok)
    ESP_LOGW(TAG, "Write to %s failed", path);
  return ok;
}

// Appends to /sdcard/<yyyymmdd>.CSV (8.3 name: long file names are off in ESP-IDF's FATFS by default)
// and writes the header first when the file is new. The file is closed after every row, so a card
// pulled between writes keeps a consistent file. On a write error the card is remounted once and the
// row retried; if that fails too, it stays unmounted until the periodic mount retry.
inline bool do_append(const char *yyyymmdd, const char *header, const Row &row) {
  if (card == nullptr)
    return false;
  char path[32];
  snprintf(path, sizeof(path), "%s/%s.CSV", MOUNT_POINT, yyyymmdd);
  for (int attempt = 1; attempt <= 2; attempt++) {
    if (write_row(path, header, row)) {
      ESP_LOGD(TAG, "Row written to %s", path);
      if (++rows_since_free >= FREE_REFRESH_ROWS) {
        free_cached = read_free_mb();
        rows_since_free = 0;
      }
      return true;
    }
    do_unmount("Write error (unmounted)");
    if (attempt == 1 && !do_mount())
      return false;
  }
  return false;
}

inline void task_main(void *) {
  static Msg msg;  // ~540 bytes; static keeps it off the task stack
  do_mount();
  for (;;) {
    if (xQueueReceive(queue, &msg, portMAX_DELAY) != pdTRUE)
      continue;
    switch (msg.type) {
      case MsgType::MOUNT:
        do_mount();
        break;
      case MsgType::UNMOUNT:
        do_unmount("Not mounted (safe to remove)");
        break;
      case MsgType::ROW:
        do_append(msg.date, msg.header, msg.row);
        break;
    }
  }
}

// ---- called from the loop task ----

inline bool send(const Msg &m, const char *what) {
  if (queue == nullptr)
    return false;
  if (xQueueSend(queue, &m, 0) != pdTRUE) {
    ESP_LOGW(TAG, "Queue full (card busy), %s dropped", what);
    return false;
  }
  return true;
}

// Starts the sdlog task, which initializes the card's SPI bus and mounts the card. Returns immediately.
inline bool begin(int clk_gpio, int mosi_gpio, int miso_gpio, int cs_gpio) {
  if (queue != nullptr)
    return true;
  clk_pin = clk_gpio;
  mosi_pin = mosi_gpio;
  miso_pin = miso_gpio;
  cs_pin = cs_gpio;
  set_status("Mounting");
  queue = xQueueCreate(QUEUE_LENGTH, sizeof(Msg));
  if (queue == nullptr || xTaskCreate(task_main, "sdlog", TASK_STACK, nullptr, 1, nullptr) != pdPASS) {
    set_status("Task start failed");
    ESP_LOGE(TAG, "Task start failed");
    return false;
  }
  return true;
}

// Message buffer for the loop task (xQueueSend copies it); static keeps ~540 bytes off the loop stack.
inline Msg loop_msg;

inline void request_mount() {
  loop_msg.type = MsgType::MOUNT;
  send(loop_msg, "mount request");
}

inline void request_unmount() {
  loop_msg.type = MsgType::UNMOUNT;
  send(loop_msg, "unmount request");
}

// Queues one row for /sdcard/<yyyymmdd>.CSV; header is written when the file is new.
inline void log_row(const char *yyyymmdd, const char *header, const Row &row) {
  loop_msg.type = MsgType::ROW;
  snprintf(loop_msg.date, sizeof(loop_msg.date), "%s", yyyymmdd);
  loop_msg.header = header;
  loop_msg.row = row;
  send(loop_msg, "row");
}

}  // namespace sdlog
