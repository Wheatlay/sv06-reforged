# Feature checklist

Go through this after flashing (see [FIRST_BOOT.md](FIRST_BOOT.md) first). Tick each line that works; open an issue
with a photo of the screen if one doesn't.

## Menu
- [ ] Idle main menu order: Print from Media, Level Bed, Change Filament, Spools, Night Mode, Motion, Configuration
- [ ] During a print: Pause, Night Mode, Stop, Tune, Change Filament, Configuration (no Level Bed, no Motion)
- [ ] Paused: Resume instead of Pause, Stop still there, no Level Bed / Motion
- [ ] Motion > Preheat shows **PETG**; it heats to 235 / 75 with the fan at 50%

## Level Bed shortcut and heat prompts
- [ ] Level Bed from the main menu homes (if needed), heats the bed, probes 25 points and finishes without errors
- [ ] Cold printer: Level Bed asks "Heat to 120/60, then level bed?" with Proceed preselected; Back returns
- [ ] Cold printer: Auto Home (Motion, Bed Leveling) asks "Heat to 120/60, then home?"; Proceed heats both at once, then homes
- [ ] Clicking Level Bed / Auto Home again while one runs does nothing (back to the status screen)
- [ ] Move Extruder when cold asks "Hotend too cold. Heat to 185?"; Proceed heats and opens the move screen
- [ ] Heating message is a plain "Heating..." for both nozzle and bed
- [ ] After power off/on and a G28 (or a print), `M420 V` shows "Bed Leveling ON" (mesh used in prints)
- [ ] Menu font is the normal size after leaving the status screen (was 4x6 before the fix)

## Status screen
- [ ] Idle: temperatures on top, "SV06 Ready." (or the last message) in the middle, "Insert card or choose a file" at the bottom
- [ ] While heating: targets of both heaters appear, labels hidden; they stay until the heater reaches its target (within 1 degree)
- [ ] Printing: TIME LEFT and ELAPSED, both counting; progress bar with the percentage inside; file name at the bottom
- [ ] TIME LEFT roughly matches Orca's estimate (it comes from Orca's M73 lines)
- [ ] Fan icon animates while the fan runs
- [ ] Paused: bar says PAUSED and blinks

## Night mode
- [ ] Main menu > Night Mode: status line says "Night mode on", the moon appears on the status screen
- [ ] During a print: travel is clearly slower and quieter; turning it off returns to normal speed
- [ ] No beeps while night mode is on
- [ ] After power off/on night mode is off again

## Delayed start (try with a short delay first)
- [ ] Choosing a file shows: Cancel / Print now / Start later
- [ ] Start later: the knob changes the time in 30-minute steps (0:30 ... 24:00), press to confirm
- [ ] Status screen shows STARTS IN with the countdown, PRINT TAKES with Orca's estimate (or --:-- if not found), bar labelled WAITING
- [ ] Main menu shows "Cancel delayed start"; cancelling stops the countdown
- [ ] With 0:30: the print starts by itself after 30 minutes (stay nearby the first time)
- [ ] Nothing heats or moves while waiting

## Printed marks
- [ ] After a print finishes, the file shows a check mark in Print from Media
- [ ] After a second finished print of the same file it shows 2
- [ ] A print you stop does not add a mark
- [ ] Marks survive power off/on (stored in the EEPROM at byte 1536, nothing is written to the card)

## Input shaping and linear advance
- [ ] Configuration > Advanced Settings > Input Shaping shows X 40.3 Hz / Y 40.3 Hz, zeta 0.15 (values from Sovol V2.2.0 binary). Tuning guide to follow.
- [ ] `M900` shows K0.02 (Sovol V2.2.0 value)

## Spool tracker
- [ ] Main menu > Spools: set PLA left (knob changes 10 g per click), "New PLA spool 1kg" sets 1000
- [ ] Idle status screen shows "PLA 1000g  PETG --" in the bar area
- [ ] After a finished PLA print, PLA left drops by about Orca's "filament used [g]" and "PLA used" grows
- [ ] A stopped print subtracts only what was extruded
- [ ] Choosing a file that needs more than is left (+5%) shows "Low filament! 120g > 80g left" above Print now
- [ ] Values survive power off/on (EEPROM byte 1800)
