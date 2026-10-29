# FAQ

**Which printers does this work on?**
Only the original Sovol SV06: board V131 with a GD32F103RET6 chip, a 128x64 monochrome screen and a knob. Not the
SV06 Plus or the SV06 ACE, which have different boards and screens.

**Can it brick my printer?**
No. Flashing from the SD card replaces only the firmware; the bootloader that reads the card stays untouched. If
something goes wrong, flash Sovol's stock file the same way. See [FLASHING.md](FLASHING.md).

**Do I need to change my slicer profile?**
No. Sovol's OrcaSlicer profiles work as they are. Two things are worth knowing:
- Time left and progress come from the `M73` lines that OrcaSlicer writes. Until the first one arrives (heat-up,
  start G-code), the screen shows `--:--` and 0%.
- The bed mesh is applied automatically after every `G28`, so the start G-code needs no `M420 S1`.

**Does it print faster than stock?**
The firmware allows what Sovol's own "High-Speed" profile asks for (accelerations up to 5000 mm/s², 300 mm/s), with
input shaping and linear advance to keep corners clean. The actual speed comes from your slicer profile. The
values from Sovol's profile: input shaping X/Y 40.3 Hz, damping 0.15, linear advance K 0.02, jerk 20.

**Why is the default acceleration only 500?**
Those are Sovol's stock defaults for G-code that doesn't set its own limits. OrcaSlicer sends its limits at the
start of every print and the firmware follows them.

**I flashed and got an EEPROM alert. Is something broken?**
No. The stored settings from the previous firmware have a different layout. Choose Reset, wait about 10 seconds,
then enter your Z offset again and run Level Bed.

**After flashing, the printer stays on a blue screen.**
Usually the card isn't read. Power off, push the card fully into the slot until it clicks, and power on again.
Always eject the card in your operating system before pulling it out.

**Where are my settings, marks and spool data stored?**
In the printer's EEPROM. Nothing is written to the SD card.

**How does the spool tracker know how much is left?**
You tell it once: in Main menu > Spools, enter grams left for PLA and PETG, or choose "New PLA spool 1kg". After
that the firmware subtracts what the extruder actually pushed in every print. It uses the file's
`; filament_type` to pick the spool; without one, a nozzle at 220 °C or more counts as PETG. A value of 0 means
"not set": no warning for that type.

**Night mode doesn't seem to slow down my print.**
Night mode caps speed at 70 mm/s, acceleration at 600 mm/s² and jerk at 8, even when the G-code asks for more. Turn it
on from the main menu or during a print (right under Pause). It resets to off at power-up.

**Temperatures behave differently from stock.**
Marlin 2.1 regulates heaters differently from the 2.0 that Sovol's PID values were tuned for. Run a PID autotune
once: `M303 E0 S210 C8 U1`, then `M303 E-1 S60 C8 U1`, then `M500`.

**Where's the About Printer menu?**
It was removed to make room in flash. The version (`2.1.2.9`) shows on the boot screen.

**How do I build it myself?**
See "Building" in the [README](../README.md). Every build must pass `tools/safety_check.py` before it goes on a
printer.
