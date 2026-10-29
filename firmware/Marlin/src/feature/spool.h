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
 * SV06 spool tracker: grams of filament left per type (PLA, PETG), counted from the net
 * extrusion of each job and kept in the EEPROM. The type comes from the file's
 * "; filament_type" comment, or from the hotend temperature if the file has none.
 */

#include "../inc/MarlinConfig.h"

enum SpoolType : int8_t { SPOOL_NONE = -1, SPOOL_PLA = 0, SPOOL_PETG = 1, SPOOL_COUNT = 2 };

class SpoolTracker {
public:
  static uint16_t grams_left[SPOOL_COUNT];   // 0 = not set (no warning, shown as "--")
  static uint32_t grams_used[SPOOL_COUNT];   // totals since the last reset
  static bool counting;                      // true while a job runs; the planner adds to extruded_mm
  static float extruded_mm;                  // net filament length pushed during the current job
  static int8_t pending_type;                // set when a file is chosen in the menu (from the file), SPOOL_NONE if unknown
  static uint16_t pending_grams;             // grams the chosen file needs, 0 = unknown

  static void load();                        // EEPROM -> RAM, at boot
  static void save();                        // RAM -> EEPROM
  static void task();                        // From idle(): detects job start/end
  static void file_chosen(const char *abs_path); // Read slicer info into pending_*
  static bool pending_low();                 // The chosen file needs more than is left (with 5% margin)
  static int8_t type_from_name(const char *s);
  static void new_spool_pla();               // grams_left[PLA] = 1000
  static void new_spool_petg();              // grams_left[PETG] = 1000
  static void reset_used();                  // grams_used[] = 0

private:
  static int8_t job_type;                    // Type for the running job (from pending_type at job start)
  static celsius_t max_hotend_target;        // Highest hotend target seen during the job (fallback type)
};

extern SpoolTracker spool;
