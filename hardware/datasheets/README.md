# Datasheets

Manufacturer datasheets for the sensors on the box. Archived here so the operating ranges quoted in
[`docs/sensor-capability-matrix.md`](../../docs/sensor-capability-matrix.md) stay traceable.

Downloaded from the vendor sites, unmodified. If a chip is replaced, re-check the document.

| File | Chip | Document | Source |
| --- | --- | --- | --- |
| `Sensirion_SHT4x.pdf` | SHT41 | Datasheet SHT4x, V7.3 | <https://sensirion.com/media/documents/33FD6951/6A7C10A0/HT_DS_Datasheet_SHT4x_V7.3.pdf> |
| `Sensirion_SCD4x.pdf` | SCD40 | SCD4x datasheet, Version 1.3, 2022-09 | <https://sensirion.com/media/documents/E0F04247/631EF271/CD_DS_SCD40_SCD41_Datasheet_D1.pdf> |
| `Sensirion_SGP30.pdf` | SGP30 | Datasheet SGP30, Version 1.0 – May 2020 – D1 | <https://sensirion.com/media/documents/984E0DD5/61644B8B/Sensirion_Gas_Sensors_Datasheet_SGP30.pdf> |
| `Sensirion_SGP4x.pdf` | SGP41 | Datasheet SGP41, Version 1.0, 2021-12 | <https://sensirion.com/media/documents/5FE8673C/61E96F50/Sensirion_Gas_Sensors_Datasheet_SGP41.pdf> |
| `Bosch_BME680.pdf` | BME680 | BST-BME680-DS001-09, Rev 1.9, 2024-02 | <https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme680-ds001.pdf> |
| `LiteOn_LTR-390UV.pdf` | LTR390 | LTR-390UV-01, Spec DS86-2015-0004, 2016-02-03 | <https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV_Final_%20DS_V1%201.pdf> |
| `ScioSense_ENS160.pdf` | ENS160 | ENS160 datasheet, Version 1.3, 2023-03-29 | <https://www.sciosense.com/wp-content/uploads/2023/12/ENS160-Datasheet.pdf> |
| `Winsen_ZE08-CH2O.pdf` | ZE08-CH2O | User's Manual, Version 1.7, valid from 2020-04-17 | <https://www.winsen-sensor.com/d/files/manual/ze08-ch2o.pdf> |
| `Plantower_PMS5003.pdf` | PMSx003 | PMS5003 data manual, v2.3 (English) | <https://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf> |
| `Asair_AHT21.pdf` | AHT21 | Data Sheet AHT21, ASAIR V1.0.03, 2021-05 | <https://asairsensors.com/wp-content/uploads/2021/09/Data-Sheet-AHT21-Humidity-and-Temperature-Sensor-ASAIR-V1.0.03.pdf> |

## Not archived

- **SR602 (PIR module)** — generic breakout, no manufacturer datasheet exists. The module manual (as
  translated on <https://www.sinoning.com/hc-sr602-datasheet/>) and the write-up on
  <https://www.codrey.com/electronics/mh-sr602-pir-motion-sensor-guide/> are the closest sources:
  - three connections: `+` (3.3 – 15 V), `−` (GND), `OUT` (high on motion).
  - **two extra solder pads for an optional F3 photodiode** — fitted, the module only triggers at night.
    No photodiode is populated on the board.
  - output time (2.5 s … 1 h, default 2.5 s) and sensitivity are **not** jumpered; they are set by
    swapping an SMD resistor. Blocking time is fixed at 2 s.
