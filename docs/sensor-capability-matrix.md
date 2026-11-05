# Sensor Capability Matrix

Which physical sensor outputs which measurement. Based on `esphome/airquality.yaml`.
Derived/template values (dewpoint, absolute humidity, "Best Temperature", IAQ classification, system metrics)
are left out — this only covers the sensor chips themselves.

| Measurement | SHT41 | SCD40 | SGP30 | SGP41 | LTR390 | AHT21 | ENS160 | BME680 | ZE08-CH2O | PMSx003 | SR602 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Bus / interface | I²C1, 0x44 | I²C1, 0x62 | I²C1, 0x58 | I²C1, 0x59 (not populated) | I²C1, 0x53 | I²C2, 0x38 | I²C2, 0x53 | I²C1, 0x77 | UART | UART | GPIO39, digital |
| Temperature | ✕ | ✕ | | | | ✕ | | ✕ | | | |
| Humidity | ✕ | ✕ | | | | ✕ | | ✕ | | | |
| Pressure | | | | | | | | ✕ | | | |
| CO2 (direct) | | ✕ | | | | | | | | | |
| eCO2 (estimated) | | | ✕ | | | | ✕ | | | | |
| TVOC / VOC index | | | ✕ | ✕ | | | ✕ | | | | |
| NOx index | | | | ✕ | | | | | | | |
| CH2O | | | | | | | | | ✕ | | |
| PM1.0 | | | | | | | | | | ✕ | |
| PM2.5 | | | | | | | | | | ✕ | |
| PM10 | | | | | | | | | | ✕ | |
| Ambient light | | | | | ✕ | | | | | | |
| UV index | | | | | ✕ | | | | | | |
| Gas resistance | | | | | | | | ✕ | | | |
| Motion | | | | | | | | | | | ✕ |

## Notes

- **eCO2** (SGP30, ENS160) is a VOC-based CO2 *estimate*, not a direct CO2 measurement — only the SCD40 measures CO2 directly (NDIR).
- **SGP41** is configured but not physically populated (SGP30 is installed instead); its columns describe what the chip supports, not what it currently outputs. The `sgp4x` block stays in the config, so each boot logs `Get serial number failed` and marks the component FAILED; its two entities stay unavailable.
- Several sensors provide more than one bus-native channel not listed as separate columns here (e.g. BME680's gas resistance feeds an IAQ estimate, ENS160's AQI is derived from its own eCO2/TVOC).
