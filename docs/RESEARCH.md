# Research notes (29.10.2026)

Sources read in full: all 29 pages of the SV06 knowledge base (sv06.blakadder.com, incl. SV06 Plus pages),
Sovol wiki SV06 page (wiki.sovol3d.com/en/SV06), Sovol's V2.2.0 package (firmware, tutorial, example G-code),
hillsoftware/sv06 issues, Sovol forum threads on V2.0.0A_2.24.

## Sovol firmware versions

| Version | Date | Base | Source published | Notes |
|---|---|---|---|---|
| V2.0.0_A (`SV06V2.0.0A_2.24-BCI.bin`) | 24.02.2023 | Marlin 2.0.9.2 | Yes (`sovol-stock/`) | **On the printer.** Rollback file in `stock/` |
| V2.0.0_A_F | 19.06.2023 | Marlin 2.0.9.2 | No | Same + filament runout sensor |
| V2.2.0 "Marlin High Speed" | 16.05.2024 | reports "Marlin 2.2.0" | **No** (repo stops at V2.0.0_A) | Input shaping, X-twist, linear advance, runout, power-loss recovery. Paired with Orca profile "0.20mm High-Speed @Sovol SV06" |

V2.2.0 is 235,116 bytes and V2.0.0_A 233,988 bytes, both above the community "228 KB" limit, so that limit is
approximate. Our gate (233,472) stays as a conservative margin. All three start at 0x08007000 (vector table checked).

Because V2.2.0's source isn't published, it can't be the base for our features; our build has the same feature set
(input shaping, linear advance, power-loss recovery) from upstream Marlin. X-twist is deliberately not enabled
(community crash report, see plan).

## Key finding: the slicer profile sets the motion limits

Tested with Sovol's high-speed Orca profile. Its G-code starts with:

```
M201 X5000 Y5000 Z500 E5000   ; max acceleration (firmware default 500)
M203 X300 Y300 Z10 E30        ; max speed
M204 P5000 R1000 T5000        ; print / retract / travel acceleration
M205 X20 Y20 Z0.4 E2.5        ; jerk (firmware default 8)
```

then prints with `M204 S2500` / `S1250`, travel 150 mm/s (F9000), walls 100 to 150 mm/s. Marlin accepts these, so
the printer already runs far above the firmware defaults, also on stock V2.0.0_A.

Consequences:
- **Night mode must clamp incoming M201/M203/M204/M205 and feedrates**, not just set planner values once, or the next
  print's start G-code overrides it. With 2500 mm/s² / 150 mm/s / jerk 20 as the normal case, the "strong" limits
  (travel 70 mm/s, accel 600) are meaningful again, plus a jerk cap (proposal 8, Sovol's own default).
- Input shaping matters more than expected (2500 mm/s² printing) and is worth calibrating.
- The firmware defaults only apply to moves outside a sliced print (menu moves, homing).

Start G-code relies on the firmware for the bed mesh (no M420/G29): `RESTORE_LEVELING_AFTER_G28` and
`STARTUP_COMMANDS "M420 S1"` are on in our build, same as Sovol.

## Hardware facts (knowledge base)

- Hotend thermistor: glass bead NTC 100K 3950 glued in the block; Sovol's firmware uses a fitted custom curve
  (`TEMP_SENSOR_0 1000`, R25 94162, beta 4160), kept as is. Bed: NTC 100K 3950 (`TEMP_SENSOR_BED 1`).
- Probe: inductive, needs a steel sheet. Screen: standard Ender-3 type 12864 LCD.
- X/Y motors NEMA17 42-34 1.3 A. 10 LM8UU bearings (3 X, 4 Z, 3 Y).
- Beeper on PC6 (from the Klipper macro).
- Nozzle: MK8 M6 thread (Creality Sprite type).

## Noise (relevant to night mode)

- **Dry bearings** are a known factory issue: Sovol ships LM8UU bearings without grease, which gives the "walking on
  gravel" sound during travel. Quick fix: lubricate the rods (PTFE dry lube, lithium grease). Permanent fix: pack the
  bearings with grease (videos by lost in tech / SavageLau, linked on the knowledge base).
- Power supply fan: printable 70/80 mm silent fan covers exist (Printables @felixna, @crasbe).
- The 4010 part-cooling and hotend fans are loud at 100%; 5015 upgrades exist (needs new duct).

## Practical tips worth acting on

- Sensorless homing: owners often raise the values in the menu (68 to 75+). Note the current values before resetting
  settings after flashing (added to FIRST_BOOT).
- Flashing card: ≤ 16 GB, FAT32 4096-byte clusters. Sovol's format command: `format /q H: /fs:fat32 /a:4096`.
  Firmware the only file, unique name, `.bin` (capital `.BIN` helps on some units).
- Blue screen: Sovol suggests operating the screen off the frame (static interference), reformatting, reflashing.
- Grub screw on the extruder gear loosens often (under-extrusion, skipping); a drop of thread locker helps.
- Heated bed cable must be out from under the bed, or homing Y fails.
