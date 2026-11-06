# Thresholds and control settings

How the box judges a reading: the colour steps on the display, the limits that sound the buzzer, and the
limits that drive the optional air filter. These are decisions of the firmware, not capabilities of the
sensors — the sensor side is in [sensor-capability-matrix.md](sensor-capability-matrix.md).

All of them refer to short-term indoor exposure.

## Colours and alarm limits

The colour steps and the alarm limits are anchored to published guideline values instead of round numbers.

| Value | Colour steps (yellow / orange / red / purple) | Basis |
| --- | --- | --- |
| CO2 (SCD40) | 1000 / 1500 / 2000 / 3000 ppm | 1000 ppm is the long-standing "hygienically unobjectionable" indoor value (Umweltbundesamt, health assessment of CO2 in indoor air) and the ventilation trigger of ASR A3.6, the workplace rule issued under the German Workplace Ordinance; above 2000 ppm ASR A3.6 calls the air unacceptable. The occupational exposure limit is 5000 ppm over 8 h. |
| PM2.5 | 15 / 25 / 50 / 75 µg/m³ | WHO global air quality guidelines (2021): 15 µg/m³ as the 24 h guideline, 25 / 37.5 / 50 / 75 as its interim targets 4 / 3 / 2 / 1. |
| PM10 | 45 / 50 / 75 / 100 µg/m³ | WHO 2021: 45 µg/m³ as the 24 h guideline; the EU daily limit value is 50 µg/m³ (35 exceedances per year allowed); 75 / 100 are WHO interim targets. |
| PM1 | no colour | No guideline value exists for PM1, so the value stays neutral. |
| CH2O (ZE08) | 80 / 120 / 200 / 400 ppb | 0.1 mg/m³ is the WHO indoor air guideline (30 min average) and the German indoor air value; at 20 °C that is 80 ppb. The further steps are 1.5x, 2.5x and 5x of it. |
| TVOC (ENS160, SGP30) | 400 / 800 / 1400 / 2000 ppb | There is no health-based limit: TVOC is a sum parameter, and ppb converts to µg/m³ only with a known reference gas (factor 1.9 … 3.8). These steps are the room's own trend bands, not a health scale. |

Alarm (buzzer): CO2 above its threshold (default 2000 ppm), PM2.5 above 15 µg/m³ and PM10 above 45 µg/m³.
The condition has to hold for 5 minutes before the buzzer sounds. CH2O and TVOC never trigger it — the
ZE08-CH2O is a low-cost electrochemical sensor with drift and a fixed offset, and TVOC has no limit to
compare against. Both eCO2 values (ENS160, SGP30) are derived from VOC readings rather than measured, so they
are shown as `(E)CO2` for comparison and never drive a colour step or an alarm on their own.

The Winsen datasheet gives no accuracy figure at all — only a resolution of ≤0.01 ppm — its protocol has no
zero-point command, and the manufacturer warns that the zero drifts after prolonged exposure to organic gases.
The reading is therefore a relative value rather than a measurement: "CH2O Offset" is subtracted in the
sensor filter, and "Zero CH2O (fresh air)" sets it from the current reading, so the colour steps start at 0
after a fresh-air calibration.

The datasheet's own cross-sensitivity table explains the surprises: 100 ppm ethanol reads like 40 ppm
formaldehyde, 10 ppm toluene like 0.46 ppm, 50 ppm H2S like 3 ppm and 200 ppm CO like 0.64 ppm. Disinfectant,
perfume, alcohol or a freshly opened bottle therefore produce large false values, and no offset removes them —
the offset only shifts the baseline. Winsen also advises against using the module in systems related to human
safety, which is why CH2O is shown but never allowed to sound the buzzer.

## Further colour steps

These colour a value on the display but never sound the alarm.

| Value | Colour steps (yellow / orange / red / purple) | Basis |
| --- | --- | --- |
| AQI (ENS160) | level 2 / 3 / 4 / 5 | The ENS160 reports a five-level index of its own (1 excellent, 2 good, 3 moderate, 4 poor, 5 unhealthy), so every level above "excellent" gets its own colour. |
| IAQ (BME680, BSEC2) | 50 / 100 / 150 / 250 | Bosch's IAQ scale: 0-50 excellent, 51-100 good, 101-150 moderate, 151-200 unhealthy, 201-250 very unhealthy, above that severe to extreme. The value stays grey until BSEC2 reports `Calibrated`; the levels before that are Stabilizing, Uncertain and Calibrating. |
| UVI (LTR390) | 3 / 6 / 8 / 11 | WHO UV index: 3 is the top of "moderate", 6 "high", 8 "very high", 11 "extreme". Indoors, behind a window, the value is close to zero most of the time. |
| Dew point (BME680) | 10 / 12 / 16 °C | The coldest inner surfaces of a heated room — wall corners, the area behind furniture — sit at 10-12 °C. Once the dew point reaches that range, moisture condenses there and mould can grow. Above 16 °C the air is sultry and condenses on almost any surface. |
| Absolute humidity (BME680) | 5 / 9.4 / 10.7 / 13.6 g/m³ | The same scale as the dew point, because absolute humidity alone determines it: 9.4 / 10.7 / 13.6 g/m³ are the saturation densities at a dew point of 10 / 12 / 16 °C. Below 5 g/m³ the value turns grey — roughly 32 % relative humidity at 21 °C, dry air. The recommended indoor range of 40-50 % RH at 21-22 °C corresponds to 7.3-9.7 g/m³. |
| Pressure tendency (BME680) | 0.1 / 1.5 / 3.5 / 6.0 hPa per 3 h | The WMO synoptic code for the three-hour pressure tendency: up to 0.1 steady, up to 1.5 a slight rise or fall, up to 3.5 rising or falling, up to 6.0 rapidly, above that very rapidly — the last two are what accompanies a weather change. The sign carries the direction: negative is falling (a low is approaching), positive is rising (it is clearing up). Coloured by magnitude, so a rapid rise is highlighted just like a rapid fall. |

There is no health-based limit for absolute humidity — the g/m³ steps are a building-physics criterion (condensation
on cold surfaces), not an exposure limit. Note that relative humidity alone says little about mould risk: 60 % in a
cold room carries more water than 60 % in a warm one, which is why the box judges the dew point and the absolute
humidity instead.

The pressure view shows QNH — the pressure reduced to sea level, which is the number weather reports quote — once
"Height Above Sea Level" is set. That height is a constant taken from a map, so unlike a manually entered sea-level
pressure it does not have to be corrected whenever the weather changes. Until it is set, the view shows the plain
local reading. Nothing in the firmware depends on the QNH: the SCD40 is compensated with the local pressure
directly, and the pressure graphs plot the local reading as well. The 3 h tendency needs no reference at all — it
is the difference to the reading three hours ago, so indoor and outdoor pressure being equal is what makes it a
weather signal.

## Air filter control

The fan-speed limits for the optional air filter are control settings rather than health limits: the fan
starts one colour step before the display warns and ramps to full speed at the red step. Defaults: speed 1
above 10 µg/m³ PM2.5 (the display turns yellow at 15) and above 800 ppb TVOC (the display's orange step), full
speed above 50 µg/m³ PM2.5 and above 2000 ppb TVOC. Speeds 2-5 sit between the two numbers at equal ratios, and
the higher of the two values wins.

These numbers are restored from flash. Editing the default in the configuration only affects a device that has
no stored value yet, so change them once in the web UI and they persist. The button "Filter Thresholds Reset"
writes the four defaults back and re-evaluates the fan speed immediately.
