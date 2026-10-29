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
 * Printed marks: remembers which files were printed to the end.
 * Stored in the printer's EEPROM (not on the card), one hash + count per file name.
 */

#include "../inc/MarlinConfigPre.h"

#include <stdint.h>

#ifndef PRINTED_MARKS_MAX
  #define PRINTED_MARKS_MAX 48
#endif

class PrintedMarks {
public:
  static void load();                               // Read the marks from the EEPROM (called after the card mounts)
  static void print_started();                      // Remember the hash of the file being printed
  static void print_finished();                     // Count +1 for that file and save to the EEPROM
  static uint8_t count_for(const char *longname);   // 0 = never printed

private:
  struct Entry { uint32_t hash; uint8_t count; };
  static Entry entries[PRINTED_MARKS_MAX];
  static uint8_t used;
  static uint32_t current_hash;                     // 0 = none
  static uint32_t hash(const char *s);
  static void save();
};

extern PrintedMarks printed_marks;
