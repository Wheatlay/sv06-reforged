/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * SV06: reads the slicer's summary comments from the last 20 KB of a G-code file on the card.
 * Shared by Delayed start (print time) and the spool tracker (grams, filament type).
 */

#include "../inc/MarlinConfig.h"

struct SlicerInfo {
  uint16_t minutes;   // "; estimated printing time (normal mode) = 1d 2h 3m 4s" -> minutes rounded up, 0 = unknown
  uint16_t grams;     // "; filament used [g] = 11.33" -> rounded to nearest gram, 0 = unknown
  char type[8];       // "; filament_type = PLA" (first value, trimmed), "" = unknown
};

// Uses a local file handle, so the card's print state is not touched. False if the file can't be opened.
bool read_slicer_info(const char *abs_path, SlicerInfo &info);
