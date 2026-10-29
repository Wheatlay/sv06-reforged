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
 * SV06 spool tracker: up to 8 user-defined spools (name, density, grams left, grams used).
 * One of them is "loaded" (chosen in the menu or with M1100 A); the net extrusion of each
 * job is counted against it. Kept in the EEPROM.
 */

#include "../inc/MarlinConfig.h"

#define SPOOL_MAX       8
#define SPOOL_NAME_LEN  10   // chars, without the terminator

enum SpoolMaterial : uint8_t { MAT_OTHER = 0, MAT_PLA, MAT_PETG, MAT_ABS, MAT_ASA, MAT_TPU, MAT_COUNT };

const char* material_name(uint8_t m);               // "Other", "PLA", ...
uint8_t material_from_name(const char *s);          // case-insensitive prefix, 0xFF = unknown
uint8_t material_density_cg(uint8_t m);             // typical density x100

struct Spool {
  char     name[SPOOL_NAME_LEN + 1]; // "" = empty slot
  uint8_t  material;                 // SpoolMaterial
  uint8_t  density_cg;               // g/cm3 x100, e.g. 124 = 1.24
  uint16_t grams_left;               // 0 = not set (no warning, shown "--"), 1 = empty
  uint32_t grams_used;
  uint8_t  carry_cg;                 // 0-99, fraction of a gram carried to the next print
};

class SpoolTracker {
public:
  static Spool spools[SPOOL_MAX];
  static uint8_t active;             // index of the loaded spool (always a non-empty slot)
  static bool counting;              // true while a job runs; the planner adds to extruded_mm
  static float extruded_mm;          // net filament length pushed during the current job
  static uint16_t pending_grams;     // grams the chosen file needs, 0 = unknown
  static int8_t pending_spool;       // spool chosen for the chosen file, -1 = none
  static uint8_t pending_material;   // file's "; filament_type" as SpoolMaterial, 0xFF = unknown

  static void load();                // EEPROM -> RAM, at boot
  static void save();                // RAM -> EEPROM
  static void task();                // From idle(): detects job start/end
  static void file_chosen(const char *abs_path); // Read slicer info into pending_*
  static void choose();              // Pick pending_spool from pending_material
  static uint8_t candidates();      // Non-empty spools of the file's material (0 if unknown)
  static bool pending_low();         // The chosen file needs more than is left on the pending spool (5% margin)
  static bool pending_mismatch();    // The file's material is known and differs from the pending spool's
  static void new_spool_1kg();       // Loaded spool: grams_left = 1000
  static void reset_used();          // Loaded spool: grams_used = 0
  static void select(uint8_t i);     // Load slot i (if not empty)
  static Spool& cur() { return spools[active]; }
};

extern SpoolTracker spool;
