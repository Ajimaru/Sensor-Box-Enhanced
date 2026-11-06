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

## Operating ranges

Taken from the manufacturer datasheets archived in [`hardware/datasheets/`](../hardware/datasheets/)
(document versions and source URLs are listed there). "Operating" is what the datasheet allows at all,
"Recommended" is where the accuracy statements hold.

| Sensor | Operating temperature | Operating humidity | Recommended / accuracy conditions | Datasheet |
| --- | --- | --- | --- | --- |
| SHT41 | −40 … +125 °C | 0 … 100 %RH, condensing allowed | best performance 5 … 60 °C, 20 … 80 %RH | [SHT4x V7.3](../hardware/datasheets/Sensirion_SHT4x.pdf) |
| SCD40 | −10 … +60 °C | 0 … 95 %RH, non-condensing | CO2 ±(50 ppm + 5 %) over 400 … 2000 ppm, specified at 15 … 35 °C / 20 … 65 %RH | [SCD4x v1.3](../hardware/datasheets/Sensirion_SCD4x.pdf) |
| SGP30 | −40 … +85 °C | 10 … 95 %RH, non-condensing | recommended 5 … 55 °C and 4 … 30 g/m³ absolute humidity | [SGP30 v1.0 D1](../hardware/datasheets/Sensirion_SGP30.pdf) |
| SGP41 (not fitted) | −20 … +55 °C | 0 … 90 %RH, non-condensing | humidity compensation recommended across the whole range | [SGP41 v1.0](../hardware/datasheets/Sensirion_SGP4x.pdf) |
| LTR390 | −40 … +85 °C | not specified in the datasheet | — | [LTR-390UV-01](../hardware/datasheets/LiteOn_LTR-390UV.pdf) |
| AHT21 | −40 … +120 °C | 0 … 100 %RH (best 0 … 80 %) | accuracy ±0.3 °C / ±2 %RH typ at 25 °C; above 80 %RH the reading drifts and recovers slowly | [AHT21 V1.0.03](../hardware/datasheets/Asair_AHT21.pdf) |
| ENS160 | −40 … +85 °C | 5 … 95 %RH, non-condensing | recommended −5 … +60 °C / 20 … 80 %RH; accuracy at 25 °C / 50 %RH | [ENS160 v1.3](../hardware/datasheets/ScioSense_ENS160.pdf) |
| BME680 | −40 … +85 °C | 0 … 100 % r.H. | pressure 300 … 1100 hPa | [BST-BME680-DS001-09](../hardware/datasheets/Bosch_BME680.pdf) |
| ZE08-CH2O | −20 … +50 °C | 15 … 90 %RH, non-condensing | detection 0 … 5 ppm; storage 0 … 25 °C | [ZE08-CH2O v1.7](../hardware/datasheets/Winsen_ZE08-CH2O.pdf) |
| PMSx003 | −10 … +60 °C | 0 … 99 %RH, non-condensing | 30 s after waking from sleep before the fan delivers stable data | [PMS5003 v2.3](../hardware/datasheets/Plantower_PMS5003.pdf) |
| SR602 | not specified | not specified | generic PIR module, no manufacturer datasheet; two spare pads take an optional F3 photodiode for day/night | — |
