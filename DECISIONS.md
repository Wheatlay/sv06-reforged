# SV06 Reforged: decisions

Printer: original Sovol SV06 (board `BOARD_SOVOL_V131`, STM32F103-class, 128x64 mono LCD + knob).
Slicer: OrcaSlicer (writes M73 progress/time lines).

## Base

- Build on upstream **Marlin 2.1.2.9** (stable release) + official SV06 config from `MarlinFirmware/Configurations`.
  Not on a dev branch: hillsoftware's v60 was pulled over extruder problems from an unstable 2.1.x dev branch.
- Before flashing: compare our config with Sovol's (steps/mm, motor current, thermistors, probe offset).
  Sovol's source for comparison: `sovol-stock/` (Sovol3d/Sv06-Source-Code, Marlin 2.0.9.2, last update 18.03.2023).
- Note: the status logo bitmap in Sovol's published source is not the "SV06" logo on the printer, so the printer's
  shipped firmware differs slightly from the published code.

## Config port findings (29.10.2026)

- Printer firmware is Sovol V2.0.0_A = last commit of `sovol-stock/`, so Sovol's config is the ground truth.
- Sovol's motion values are much gentler than the official SV-06 example; we use Sovol's: accel 500 (travel 1000),
  max accel X/Y 500, max feedrate 500/500/10/30, jerk 8/8/0.4/5, probe offset {25,-25,0}.
- Firmware size limit ~228 KB (233472 bytes): GD32 runs the upper 256 KB of flash too slowly (hillsoftware #18).
- Night mode: Sovol's Orca profile ("0.20mm High-Speed @Sovol SV06") sends `M201 X5000`, `M204 P5000 T5000`,
  `M205 X20` at print start and prints at 2500 mm/s², travel 150 mm/s. So night mode must **clamp** incoming
  M201/M203/M204/M205 and feedrates. With that as the normal case the "strong" limits stay as chosen:
  travel ≤ 70 mm/s, accel ≤ 600 mm/s², plus jerk ≤ 8. See docs/RESEARCH.md.
- Input shaping is off for milestone A (stock-equivalent); enabled as a Phase 2 feature.

## Features

| Feature | Decision |
|---|---|
| Preheat ABS -> PETG | **Built**: 235° / 75° / fan 50% |
| Time left from Orca's M73 | **Built** (M73 P/R, shown on the stock status screen for now) |
| Night mode | **Built** (branch `features`): menu toggle in Main and Tune, planner caps 70 mm/s, 600 mm/s², XY jerk 8, mutes the beeper, off at boot |
| Delayed start ("start in X h", knob in 30 min steps, cancelable) | **Built** (branch `features`): Print now / Start later after choosing a file, max 24 h, countdown in the status line, cancel in main menu |
| Night mode auto-on with delayed start | No |
| Quiet ending | No |
| Mark already-printed files in the SD file list | Yes: ✓ after one finished print, a number after more. Stored in `/PRINTED.TXT` on the card |
| Input shaping | **Built**: X/Y 40.3 Hz, zeta 0.15 (read from Sovol V2.2.0 binary, MarlinSettings::reset at 0x1f620) + menu. Calibration guide still to write |
| Reset settings after first flash + checklist (Z offset, mesh) | Yes |
| Keep stock .bin for rollback | Yes |
| Spool tracker | Done: grams left per type (PLA, PETG) in EEPROM, type from Orca `; filament_type` or hotend target >= 220 = PETG, net extrusion counted by the planner during a job, low-filament warning at file choice |
| Menu shortcuts | Done: main menu by printer state (idle / printing / paused), Level Bed directly under Print from Media |

## Status screen (see mockup/lcd.js, preview: node tools/render_preview.js)

- Top band: stock icons trimmed to fit (nozzle, bed; fan redrawn 11x11, 2 animation frames). Current temperature;
  targets of both heaters shown underneath while either is heating: from a target change (or drift >10 deg) until it reaches the target within 1 deg.
- Normal: labels TIME LEFT / ELAPSED above the two times (same bold 9x18 font). While heating, targets take
  the label row and the divider moves down.
- Night mode: 16x16 moon between the two times.
- Progress bar with the percentage inside (inverted). Paused: label PAUSED, fill blinks.
- File name in 4x6 font at the bottom. Printer messages replace it (inverted) while printing, and replace "Ready" when idle.
- Waiting for delayed start: STARTS IN / PRINT TAKES, bar labelled WAITING.
- Time format always h:mm (also under 1 h).

## Build safety notes

- New feature sources must be listed in `firmware/ini/features.ini` (Marlin excludes `src/feature` by default).
- The maple toolchain only *warns* about missing functions; `-Wl,--error-unresolved-symbols` plus unflagging
  `-Wl,--warn-unresolved-symbols` makes it a hard error (verified with a deliberately broken build). Milestone A was
  rebuilt with this check: links cleanly, binary identical except the 4-byte build time.
