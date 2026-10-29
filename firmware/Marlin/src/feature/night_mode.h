/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 * Copyright (c) 2026 Wheatlay (SV06 Reforged)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * Night mode: caps speed, acceleration and X/Y jerk inside the planner, so limits sent by the slicer
 * (M201/M203/M204/M205, feedrates) can't exceed them, and mutes the buzzer. Off at every boot.
 */

#include "../inc/MarlinConfigPre.h"

class NightMode {
public:
  static bool enabled;

  static void changed();  // Menu callback after the value was flipped: show the new state

  static float limit_feedrate(const float fr_mm_s) { return enabled ? _MIN(fr_mm_s, float(NIGHT_MODE_MAX_FEEDRATE)) : fr_mm_s; }
  static float limit_accel(const float accel_mm_s2) { return enabled ? _MIN(accel_mm_s2, float(NIGHT_MODE_ACCEL)) : accel_mm_s2; }
  static float limit_xy_jerk(const float jerk) { return enabled ? _MIN(jerk, float(NIGHT_MODE_MAX_JERK)) : jerk; }
};

extern NightMode nightmode;
