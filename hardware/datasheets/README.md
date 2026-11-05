# Datasheets

Manufacturer datasheets for the sensors on the box. Archived here so the operating ranges quoted in
[`docs/sensor-capability-matrix.md`](../../docs/sensor-capability-matrix.md) stay traceable.

Downloaded 2026-11-05 from the vendor sites, unmodified. If a chip is replaced, re-check the document.

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

## Not archived

- **AHT21 (Aosong / Asair)** — Aosong lists AHT20, AHT25, AHT30 and AHT40 in its product index but
  serves no AHT21 file (the download path answers 404). Product index:
  <https://www.aosong.com/en/Products/list.aspx?lcid=139>. The box drives the chip with the AHT20
  protocol (`variant: AHT20` in `airquality.yaml`).
- **PMSx003 (Plantower)** — `plantower.com` answers 403 to automated requests, including its own
  datasheet paths, so no PDF could be archived. Product page:
  <https://www.plantower.com/en/products_33.html>.
- **SR602 (PIR module)** — generic breakout sold without a manufacturer datasheet.
