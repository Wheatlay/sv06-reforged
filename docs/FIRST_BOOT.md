# First boot checklist

Do this with the printer in sight and a hand near the power switch. If anything moves the wrong way or makes a
grinding noise, switch off immediately.

## 0. Before the first custom flash

- [ ] Flash Sovol's stock V2.0.0_A file once (see [FLASHING.md](FLASHING.md)) and check it boots. This proves the
  way back works.
- [ ] Write down your current Z offset (Bed Leveling > Probe Z Offset, or `M851` over USB). You will enter it again.

## 1. Flash and reset settings

- [ ] Flash the SV06 Reforged `.bin` with a new file name. The boot logo appears, then `2.1.2.9-R1.2`.
- [ ] If an EEPROM alert appears, choose Reset (about 10 s, don't power off). Otherwise do Configuration >
  Restore Defaults, then Store Settings. Settings saved by the stock firmware use a different layout, so this step
  is not optional.

## 2. Temperatures (no movement yet)

- [ ] Room temperature shown for nozzle and bed is plausible (about 20 to 25 °C for both).
- [ ] Preheat PLA: nozzle reaches 185 °C, bed 60 °C, no error. Then cool down.

## 3. Homing

- [ ] Move the nozzle to the middle by hand (printer powered, motors off), then Auto Home.
- [ ] The nozzle moves **left**, the bed moves **towards the front**, then Z comes down in the middle and probes.
- [ ] Homing is quiet and stops softly at the ends. A loud bang means the sensorless homing sensitivity doesn't suit
  your printer: stop, then raise it a little (default is Sovol's X 68 / Y 70, e.g. `M914 X72 Y74`, then `M500`).

## 4. Z offset and bed mesh

- [ ] Enter your Z offset (Bed Leveling > Probe Z Offset), check it with a paper test, Store Settings.
- [ ] Level Bed (main menu): all 25 points probe without the nozzle leaving the bed or belts skipping.

## 5. Test print

- [ ] Print a 20 mm calibration cube in PLA. The first layer sticks, nothing sounds unusual.
- [ ] Print something taller than 5 cm.

Optional: run a PID autotune for your heaters (`M303 E0 S210 C8 U1`, then `M303 E-1 S60 C8 U1`, then `M500`).
Marlin 2.1 regulates differently from the 2.0 that Sovol's values were tuned for.
