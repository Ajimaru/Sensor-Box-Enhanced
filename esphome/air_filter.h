#pragma once
// Air filter control without Home Assistant: sets the fan speed of an ESPHome air filter node through its
// web_server REST API (POST /number/<name>/set?value=N, HTTP Basic auth). A FreeRTOS task sends the request,
// so the loop never waits for the network; an offline filter would otherwise stall the display for the
// connect timeout on every reading.
//
// Fan speed per measured value (speed_for, steps_text): speed 1 above `start`, speed 6 above `full`, speeds 2-5 in
// between at equal ratios (start 5, full 50: 5 / 8 / 13 / 20 / 32 / 50), since concentrations spread
// logarithmically. A speed drops only once the value is about 10 % below its threshold.
//
// Used from the YAML: the filter_control script (speed_for, start), the "Filter Steps" text sensor (steps_text)
// and a 1 s interval (state, status_text).

#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esphome/core/alloc_helpers.h"
#include "esphome/core/log.h"

namespace airfilter {

static const char *const TAG = "airfilter";

// Threshold of speed 1-6, or NAN when the value is off (start 0 or unset).
inline float threshold(float start, float full, int speed) {
  if (std::isnan(start) || start <= 0)
    return NAN;
  if (std::isnan(full) || full < start)
    full = start;
  return start * powf(full / start, (speed - 1) / 5.0f);
}

// New speed 0-6 for `value`, from the current one (hysteresis); -1 = value off or no reading.
inline int speed_for(int current, float value, float start, float full) {
  if (std::isnan(value) || std::isnan(threshold(start, full, 1)))
    return -1;
  auto level = [&](float v) {
    int n = 0;
    for (int speed = 1; speed <= 6; speed++) {
      if (v > threshold(start, full, speed))
        n++;
    }
    return n;
  };
  int up = level(value);
  int down = level(value * 1.1f);
  if (current < 0 || up > current)
    return up;
  if (down < current)
    return down;
  return current;
}

// "5 / 8 / 13 / 20 / 32 / 50 µg/m³", or "off".
inline std::string steps_text(float start, float full, const char *unit) {
  if (std::isnan(threshold(start, full, 1)))
    return "off";
  std::string out;
  char buf[16];
  for (int speed = 1; speed <= 6; speed++) {
    snprintf(buf, sizeof(buf), speed == 1 ? "%.0f" : " / %.0f", threshold(start, full, speed));
    out += buf;
  }
  return out + " " + unit;
}

enum State { IDLE, SENDING, DONE, FAILED };
inline std::atomic<int> state{IDLE};
// Written by the send task before state becomes DONE or FAILED.
inline std::string result;

inline bool busy() { return state == SENDING; }

inline const char *status_text() {
  switch (state.load()) {
    case IDLE:
      return "Idle";
    case SENDING:
      return "Sending";
    default:
      return result.c_str();
  }
}

struct Job {
  std::string host;
  std::string request;
  int speed;
  const char *reason;
};

// "" on success, otherwise the text for the status entity.
inline std::string send_job(const Job &job) {
  addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *res = nullptr;
  if (lwip_getaddrinfo(job.host.c_str(), "80", &hints, &res) != 0 || res == nullptr)
    return "Error: cannot resolve " + job.host;

  int fd = lwip_socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    lwip_freeaddrinfo(res);
    return "Error: no socket";
  }
  // Same as thermal_print.h: lwip_select (no VFS select support), non-blocking connect with a 3 s limit.
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
    timeval tv = {3, 0};
    int err = 0;
    socklen_t len = sizeof(err);
    if (lwip_select(fd + 1, nullptr, &wfds, nullptr, &tv) != 1 ||
        lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0) {
      lwip_close(fd);
      return "Error: filter not reachable";
    }
  }
  lwip_fcntl(fd, F_SETFL, lwip_fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
  timeval timeout = {3, 0};
  lwip_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (lwip_send(fd, job.request.data(), job.request.size(), 0) != (ssize_t) job.request.size()) {
    lwip_close(fd);
    return "Error: send failed";
  }
  // Only the status line counts ("HTTP/1.1 200 OK"); the rest is read and dropped until the filter closes.
  char buf[64];
  ssize_t n = lwip_recv(fd, buf, sizeof(buf) - 1, 0);
  if (n > 0) {
    buf[n] = '\0';
    char discard[64];
    while (lwip_recv(fd, discard, sizeof(discard), 0) > 0) {
    }
  }
  lwip_close(fd);
  if (n <= 0)
    return "Error: no response";
  const char *space = strchr(buf, ' ');
  int code = strncmp(buf, "HTTP/", 5) == 0 && space != nullptr ? (int) strtol(space + 1, nullptr, 10) : 0;
  if (code <= 0)
    return "Error: bad response";
  if (code == 401)
    return "Error: login rejected (HTTP 401)";
  if (code != 200)
    return "Error: HTTP " + std::to_string(code);
  return "";
}

inline void send_task(void *arg) {
  Job *job = static_cast<Job *>(arg);
  std::string err = send_job(*job);
  ESP_LOGI(TAG, "%s (speed %d by %s to %s)", err.empty() ? "Sent" : err.c_str(), job->speed, job->reason,
           job->host.c_str());
  result = err.empty() ? "OK (speed " + std::to_string(job->speed) + ", " + job->reason + ")" : err;
  delete job;
  state = err.empty() ? DONE : FAILED;
  vTaskDelete(nullptr);
}

// Sends fan speed `speed` to the filter at `host` (IP or host name). `path` is the number's set URL, e.g.
// "/number/Fan%20Speed/set"; `reason` (a string literal) names the value that set the speed, for the status.
// False while a request is still running or without a host (the next run sends again).
inline bool start(const std::string &host, const char *path, int speed, const char *reason, const std::string &user,
                  const std::string &password) {
  if (busy())
    return false;
  if (host.empty()) {
    result = "Error: no Filter Host set";
    state = FAILED;
    return false;
  }
  std::string login = user + ":" + password;
  Job *job = new Job;
  job->host = host;
  job->speed = speed;
  job->reason = reason;
  job->request = std::string("POST ") + path + "?value=" + std::to_string(speed) + " HTTP/1.1\r\nHost: " + host +
                 "\r\nAuthorization: Basic " +
                 esphome::base64_encode(reinterpret_cast<const uint8_t *>(login.data()), login.size()) +
                 "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
  state = SENDING;
  if (xTaskCreate(send_task, "airfilter", 4096, job, tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
    delete job;
    result = "Error: no task";
    state = FAILED;
    return false;
  }
  return true;
}

}  // namespace airfilter
