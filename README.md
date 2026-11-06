# Sensor-Box-Enhanced

ESPHome air quality monitor based on Tom's Sensorbox v2, with extra sensors, a formaldehyde sensor with a
buzzer alarm, SD card logging and optional screen printouts on a thermal printer. It works on its own and
connects to Home Assistant through the ESPHome API.

[![Release](https://img.shields.io/github/v/release/Ajimaru/Sensor-Box-Enhanced?sort=semver&style=flat-square&logo=github)](https://github.com/Ajimaru/Sensor-Box-Enhanced/releases/latest)
[![Release workflow](https://img.shields.io/github/actions/workflow/status/Ajimaru/Sensor-Box-Enhanced/release.yml?style=flat-square&logo=githubactions&logoColor=white&label=release)](https://github.com/Ajimaru/Sensor-Box-Enhanced/actions/workflows/release.yml)
[![License: CC BY 4.0](https://img.shields.io/badge/license-CC%20BY%204.0-lightgrey?style=flat-square&logo=creativecommons&logoColor=white)](LICENSE)
[![Last commit](https://img.shields.io/github/last-commit/Ajimaru/Sensor-Box-Enhanced?style=flat-square&logo=github)](https://github.com/Ajimaru/Sensor-Box-Enhanced/commits/main)
[![ESPHome](https://img.shields.io/badge/ESPHome-2026.10-black?style=flat-square&logo=esphome&logoColor=white)](https://esphome.io/)
[![Platform](https://img.shields.io/badge/platform-ESP32--S2-red?style=flat-square&logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32-s2)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-ESPHome%20API-41bdf5?style=flat-square&logo=homeassistant&logoColor=white)](https://www.home-assistant.io/)

## Based on

- [3D Printer Emission Sensor Array (Sensorbox v2)](https://www.printables.com/model/1079858-3d-printer-emission-sensor-array-sensorbox-v2)
  by Tom (MadeWithLayers), CC BY: enclosure, main board SBR1 r1.1 (ESP32-S2, 2.8" ILI9341 display) and the
  ESPHome config r2 ([go.toms3d.org/sbr1](https://go.toms3d.org/sbr1)) that this config started from.
- [ZE08 CH2O Sensor Mount Base + Buzzer for Sensorbox v2](https://www.printables.com/model/1219568-ze08-ch2o-sensor-mount-base-buzzer-for-sensorbox-v)
  by Simply Maker, CC BY-NC-SA: mount for the ZE08-CH2O sensor and the buzzer.
- Addon board v0.2, own design (`hardware/enhanced-addon-v0.2/`).

The print files are on the Printables pages above.

## What's added

Hardware:

- ZE08-CH2O formaldehyde sensor and a buzzer, on the mount above.
- Addon board v0.2 with an SR602 PIR motion sensor, an LTR390 UV/ambient light sensor and a BME680
  (temperature, humidity, pressure, gas). The LTR390 and BME680 breakouts share one I²C bus; the board
  connects to the main board with a 4-pin header (3V3, GND, SCL, SDA) and one wire for the PIR (GPIO39).
  Pinout: [hardware/enhanced-addon-v0.2/README.md](hardware/enhanced-addon-v0.2/README.md).

![Addon board v0.2](hardware/enhanced-addon-v0.2/Addon%20Board%20v0.2.png)

Software (`esphome/airquality.yaml`):

- Eight display pages: current values, an info page with the alarm limits, the page list and the button
  controls, 1 h and 24 h graphs (air quality, climate, system) and a QR code that opens the web UI. A short
  press on the top button shows the next page, holding it for 5 s prints the page, and the box returns to the
  values page after 30 s without a press. On the values page the top row rotates every 10 s through temperature/humidity, dewpoint/absolute humidity, pressure/altitude, light/UV index and the AQI values.
  The gas block shows formaldehyde (ZE08), ENS160 TVOC and whichever VOC sensor is fitted — SGP30 or SGP41, the board has room for one. The particles block is always drawn and shows `--` when the PMS delivers nothing;
  the other blocks appear once their sensor has usable readings, and the SGP41 NOx index would sit small in the corner.
- Graphs auto-scale where the absolute level matters less (TVOC, particles, light) and keep fixed axes for
  CO2, temperature, humidity, pressure, dewpoint, heap, PSRAM and loop time; auto-scaled graphs show the unit
  only, without end labels.
- Buzzer alarm with limits for CO2, CH2O, TVOC and particulate matter, set in the web UI.
- The display backlight follows presence (PIR) and ambient light, with three levels set in the web UI.
- Indoor air quality index from the BME680 (Bosch BSEC2).
- Temperature and humidity offsets per sensor (the sensors sit in the warm case). To calibrate, enter the
  readings of a reference thermometer/hygrometer next to the box ("Reference Temperature", "Reference
  Humidity") and press "Calibrate Temperature" or "Calibrate Humidity": the box sets all four offsets so that
  each sensor reads the reference. The SCD40 carries its own 4 °C offset in the chip (ESPHome's default, for
  self-heating); that correction happens before the box's offset, so the two never add up. The gas sensors
  (ENS160, SGP30) get their temperature/humidity compensation from the SHT41. The CO2 channel of the SCD40
  has its own button, "Calibrate SCD40 CO2 (420 ppm)": let the box run in fresh air for a few minutes, then
  press it to write 420 ppm as the reference (Sensirion forced recalibration; automatic self calibration stays
  on).
- Logging to an SD card in the display module's card slot (wired to GPIO35–38): one CSV row per minute, one
  file per day.
- Optional screen print on a thermal printer, see below.
- Optional control of an IKEA FÖRNUFTIG air filter converted to ESPHome, by PM2.5 and TVOC, see below.

## Sensors

| Sensor | Measures | Interface |
| --- | --- | --- |
| SCD40 | CO2, temperature, humidity | I²C |
| SHT41 | Temperature, humidity | I²C |
| SGP30 | TVOC, eCO2 | I²C |
| ENS160 + AHT21 | TVOC, eCO2, air quality index; temperature, humidity | I²C |
| BME680 | Temperature, humidity, pressure, gas (IAQ via BSEC2) | I²C |
| LTR390 | Ambient light, UV index | I²C |
| ZE08-CH2O | Formaldehyde | UART |
| PMSx003 | PM1, PM2.5, PM10 | UART |
| SR602 | Motion | GPIO |

The config also contains an SGP41 block (VOC and NOx index), but no SGP41 is fitted — the SGP30 does that job.
The block stays for a later upgrade, so every boot logs `Get serial number failed` and
`sgp4x.sensor is marked FAILED`, and the two SGP41 entities stay unavailable. Comment the block out if you
don't plan to fit the chip.

The PMSx003 polls every 30 s, so its fan runs continuously. Above 30 s ESPHome puts the sensor to sleep
between readings; a sleeping PMS survived an ESP reboot and only came back after a mains power cycle, so
the box keeps it awake and the values page shows `--` if it ever stops delivering.

Which sensor feeds which value: [docs/sensor-capability-matrix.md](docs/sensor-capability-matrix.md).

## Thermal printer (optional)

Hold the top button for 5 s, or press "Print Screen" in the web UI, to print the current page on a 58 mm
QR204 thermal printer. The box sends the page over WiFi as ESC/POS (raw TCP, port 9100) to an ESP32-C3 that
is wired to the printer; this bridge is a separate project,
[QR204-Printer-Bridge](https://github.com/Ajimaru/QR204-Printer-Bridge). The printer's address
("Printer Host", default `thermal-printer.local`, the bridge project's own mDNS name) and the print settings
are in the web UI. Without a printer everything else works as usual.

## Air filter (optional)

The box can set the fan speed of an IKEA FÖRNUFTIG air filter that has been converted to ESPHome (Wemos D1
mini) with [3ative/ikea-air-filter](https://github.com/3ative/ikea-air-filter). It talks to the filter
directly over WiFi, through the filter's web server REST API ("Fan Speed", 0–6), so Home Assistant isn't
needed. That config has no web server; add one with the same login as the box. `type: basic` is required: the
box sends HTTP Basic authentication (`air_filter.h`), and ESPHome switches the default to digest in 2027.1.0,
so the type must not be left implicit:

```yaml
web_server:
  port: 80
  auth:
    type: basic
    username: !secret web_server_username
    password: !secret web_server_password
```

In the box's web UI, group "Air Filter":

- "Filter Auto" switches the control on. When it is off, the fan keeps its current speed.
- "Filter Host" is the filter's IP address or host name.
- PM2.5 (µg/m³) and TVOC from the ENS160 (ppb) each have two values: "Filter Speed 1 above …" and
  "Filter Speed 6 above …" (defaults 5 / 50 µg/m³ and 500 / 2000 ppb). Speeds 2–5 lie in between and are
  shown in "Filter Steps". 0 turns a value off. The higher of the two speeds wins.
- "Filter Status" shows the result of the last request, for example `OK (speed 2, TVOC)`.

The box sends when the speed changes and otherwise every 2 minutes, so the filter gets its speed back after a
restart. A speed only drops once the value is about 10 % below its threshold. The ENS160 needs about 3
minutes after a reboot and an hour after its very first power-up; until then only PM2.5 counts. The filter's
activated carbon takes up odours and VOCs, not CO2; only fresh air lowers CO2.

## Repository

| Path | Contents |
| --- | --- |
| `esphome/airquality.yaml` | The ESPHome config |
| `esphome/sd_logger.h`, `esphome/thermal_print.h`, `esphome/air_filter.h` | SD card logging, screen printing and air filter control, included by the config |
| `esphome/secrets.example.yaml` | Template for `secrets.yaml` |
| `hardware/enhanced-addon-v0.2/` | Addon board v0.2: Gerber and drill files (EasyEDA Pro export, `AddonBoard v0.2.zip` for ordering), a render and the pinout |
| `docs/` | Sensor capability matrix |

## Setup

There is no prebuilt firmware to download. The config uses Bosch BSEC2 for the BME680, whose license doesn't
allow distributing firmware built with it, and your WiFi password, web UI login and API key are compiled into
the firmware anyway. You build it yourself with ESPHome from the files in `esphome/`.

1. Copy `esphome/secrets.example.yaml` to `secrets.yaml` and fill it in (WiFi, web UI login, API key). A
   suitable API key is 32 random bytes in base64, for example from `openssl rand -base64 32`.
2. Put `airquality.yaml`, `sd_logger.h`, `thermal_print.h`, `air_filter.h` and `secrets.yaml` in the same
   folder of your ESPHome setup, for example the ESPHome Device Builder in Home Assistant.
3. In `airquality.yaml`, adjust the time zone under `time:`. The device gets its address by DHCP; for a fixed
   address add a `manual_ip:` block under `wifi:` (or a DHCP reservation in your router).
4. Flash the box, see below. The settings (alarm limits, backlight, printer) are in the web UI at the
   device's address; the login is the one from `secrets.yaml`.

Tested with ESPHome 2026.10.

### Flashing

First time, over USB (the ESP32-S2 has no firmware from this project yet):

1. Connect the main board to the computer with a USB cable. If it isn't detected, hold the BOOT button while
   plugging it in.
2. In the Device Builder open the device and choose **Install**. If the Device Builder runs on a different
   machine than the one the board is plugged into, choose **Manual download** (factory format) and flash the
   file with [ESPHome Web](https://web.esphome.io/) in Chrome or Edge instead.
3. The first build takes several minutes.

After that, choose **Install** → **Wirelessly**; no cable needed. If the box can't reach WiFi, it opens a
fallback access point named like the device and a captive portal to enter new WiFi credentials.

## License

[CC BY 4.0](LICENSE), in line with the CC BY license of Tom's Sensorbox v2, which this project builds on.
`esphome/airquality.yaml` is a modified version of Tom's ESPHome config r2. The ZE08 mount by Simply Maker is
not part of this repository and keeps its own license (CC BY-NC-SA).

When you reuse this project, credit it as: "Sensor-Box-Enhanced by Ajimaru, based on Sensorbox v2 by Tom
(MadeWithLayers)".
