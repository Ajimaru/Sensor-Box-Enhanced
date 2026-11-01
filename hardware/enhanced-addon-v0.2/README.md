# Addon board v0.2

Carrier board for the extra sensors of Sensor-Box-Enhanced: an SR602 PIR motion sensor, an LTR390 UV/ambient
light breakout and a BME680 breakout. Own design, single board, 70 × 30 mm including the connector tab on the top edge, through-hole parts only.
Built and working with all three sensors.

![Addon board v0.2](Addon%20Board%20v0.2.png)

## Files

| File | Contents |
| --- | --- |
| `AddonBoard v0.2.zip` | Manufacturing files: Gerber layers (top/bottom copper, silkscreen, solder mask, outline, drill drawing), plated drill file and flying probe test data. |
| `Addon Board v0.2.png` | Render of the top side |

## Parts

| Ref | Part | Pins |
| --- | --- | --- |
| IC1 | SR602 PIR module (5-pin) | OUT → GPIO39, + → 3V3, − → GND; the two S pads are not connected |
| U1 | LTR390 breakout (6-pin) | Vin → 3V3, GND, SCL, SDA; 3Vo and INT are not connected |
| U2 | BME680 breakout (6-pin) | 3V3, GND, SCL, SDA; CS and SDO are not connected |
| J1 | 1×4 header to the main board | 3V3, GND, SCL, SDA (pin 1 to 4) |
| P1 | 1×1 header | GPIO39, the PIR signal; connects to the main board's GPIO39 |

LTR390 and BME680 sit on the same I²C bus (J1 SCL/SDA), as in `esphome/airquality.yaml` (`i2c_1`, LTR390 at
0x53, BME680 at 0x77). Despite the `_B1`/`_B2` in the silkscreen labels, both breakouts are wired to the same
SCL and SDA nets, not to two separate buses.
