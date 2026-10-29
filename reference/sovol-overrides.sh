# Milestone A: values where Sovol V2.0.0_A (the printer's real firmware) differs from the official SV-06 example.
# Rule: never faster/harder/hotter than Sovol. Run from the project root.
C=firmware/Marlin/Configuration.h
A=firmware/Marlin/Configuration_adv.h
P="uv run --no-project tools/port_config.py set"
$P $C 'STRING_CONFIG_H_AUTHOR="(SV06 custom, based on Sovol V2.0.0_A)"' \
  'DEFAULT_MAX_FEEDRATE={ 500, 500, 10, 30 }' 'DEFAULT_MAX_ACCELERATION={ 500, 500, 100, 5000 }' \
  DEFAULT_ACCELERATION=500 DEFAULT_RETRACT_ACCELERATION=500 DEFAULT_TRAVEL_ACCELERATION=1000 \
  DEFAULT_XJERK=8 DEFAULT_YJERK=8 DEFAULT_EJERK=5 \
  'NOZZLE_TO_PROBE_OFFSET={ 25, -25, 0 }' \
  -FILAMENT_RUNOUT_SENSOR -ABL_BILINEAR_SUBDIVISION \
  Z_HOMING_HEIGHT=10 SHOW_CUSTOM_BOOTSCREEN CUSTOM_STATUS_SCREEN_IMAGE
$P $A 'HOMING_BUMP_MM={ 2, 2, 2 }' -HOMING_BACKOFF_POST_MM \
  -INPUT_SHAPING_X -INPUT_SHAPING_Y -SHAPING_MENU \
  LCD_TIMEOUT_TO_STATUS=30000 STATUS_HOTEND_INVERTED STATUS_HOTEND_ANIM STATUS_BED_ANIM \
  -EMERGENCY_PARSER X_CURRENT_HOME=X_CURRENT Y_CURRENT_HOME=Y_CURRENT Z_CURRENT_HOME=Z_CURRENT \
  -GANTRY_CALIBRATION_SAFE_POSITION -GANTRY_CALIBRATION_XY_PARK_FEEDRATE
# From the reverse check (Sovol-only settings)
$P $A WATCH_TEMP_PERIOD=20
$P $C SPEAKER 'Z_PROBE_END_SCRIPT="G28XY\nM500"'
# Required by Marlin 2.1.2.9 sanity checks: FAN_SOFT_PWM is mandatory for this board (kept on, as in the official
# example); Sovol's E-jerk 5 with Linear Advance needs ALLOW_LOW_EJERK (direct drive).
$P $A ALLOW_LOW_EJERK
# Size (GD32 limit 233472 bytes): pure diagnostics, nothing on the printer's screen. Measured savings:
# TMC_DEBUG 9440, DEBUG_LEVELING_FEATURE 4712, G26_MESH_VALIDATION 4392 bytes.
$P $C -DEBUG_LEVELING_FEATURE -G26_MESH_VALIDATION
$P $A -TMC_DEBUG
# The user's own tuned sensorless homing values from the printer (29.10.2026)
$P $A X_STALL_SENSITIVITY=65 Y_STALL_SENSITIVITY=68
# Feature build only (branch features), agreed 29.10.2026: M48 test 7096, About Printer screens 1608, PID edit menu 1584 bytes
# $P $C -Z_MIN_PROBE_REPEATABILITY_TEST -PID_EDIT_MENU
# $P $A -LCD_INFO_MENU
# SPEAKER off: maple tone() resets this board (found 29.10.2026 via M300 over USB); beeper is on/off only
$P $C -SPEAKER
