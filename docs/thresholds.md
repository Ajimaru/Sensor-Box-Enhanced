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

## Air filter control

The fan-speed limits for the optional air filter are control settings rather than health limits: the fan
starts one colour step before the display warns and ramps to full speed at the red step. Defaults: speed 1
above 10 µg/m³ PM2.5 (the display turns yellow at 15) and above 800 ppb TVOC (the display's orange step), full
speed above 50 µg/m³ PM2.5 and above 2000 ppb TVOC. Speeds 2-5 sit between the two numbers at equal ratios, and
the higher of the two values wins.

These numbers are restored from flash. Editing the default in the configuration only affects a device that has
no stored value yet, so change them once in the web UI and they persist. The button "Filter Thresholds Reset"
writes the four defaults back and re-evaluates the fan speed immediately.
