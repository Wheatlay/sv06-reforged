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

#include "../inc/MarlinConfig.h"

#if ENABLED(SPOOL_TRACKER)

#include "spool.h"
#include "slicer_info.h"
#include "../MarlinCore.h"
#include "../module/temperature.h"
#include "../HAL/shared/eeprom_api.h"

#include <string.h>

SpoolTracker spool;

uint16_t SpoolTracker::grams_left[SPOOL_COUNT]; // = 0
uint32_t SpoolTracker::grams_used[SPOOL_COUNT]; // = 0
bool SpoolTracker::counting;                    // = false
float SpoolTracker::extruded_mm;                // = 0
int8_t SpoolTracker::pending_type = SPOOL_NONE;
uint16_t SpoolTracker::pending_grams;           // = 0
int8_t SpoolTracker::job_type = SPOOL_NONE;
celsius_t SpoolTracker::max_hotend_target;      // = 0

// Printed marks use 1536..1778 (1536 + 3 + 5*48), settings end near 825.
// Layout: 'S' 'P', grams_left[2] as uint16 LE, grams_used[2] as uint32 LE (14 bytes).
#define SPOOL_EEPROM_START 1800
static_assert(SPOOL_EEPROM_START + 14 <= MARLIN_EEPROM_SIZE, "Spool tracker data does not fit in the EEPROM");
#if ENABLED(PRINTED_MARKS)
  static_assert(SPOOL_EEPROM_START >= 1536 + 3 + 5 * (PRINTED_MARKS_MAX), "Spool tracker data overlaps the printed marks");
#endif

void SpoolTracker::load() {
  uint8_t b[14];
  if (persistentStore.read_data(SPOOL_EEPROM_START, b, 14)) return;
  if (b[0] != 'S' || b[1] != 'P') return; // never saved yet
  for (uint8_t i = 0; i < SPOOL_COUNT; i++) {
    grams_left[i] = uint16_t(b[2 + 2 * i]) | uint16_t(b[3 + 2 * i]) << 8;
    const uint8_t *u = b + 6 + 4 * i;
    grams_used[i] = uint32_t(u[0]) | uint32_t(u[1]) << 8 | uint32_t(u[2]) << 16 | uint32_t(u[3]) << 24;
  }
}

void SpoolTracker::save() {
  uint8_t b[14] = { 'S', 'P' };
  for (uint8_t i = 0; i < SPOOL_COUNT; i++) {
    b[2 + 2 * i] = uint8_t(grams_left[i]);
    b[3 + 2 * i] = uint8_t(grams_left[i] >> 8);
    const uint32_t u = grams_used[i];
    b[6 + 4 * i] = uint8_t(u);
    b[7 + 4 * i] = uint8_t(u >> 8);
    b[8 + 4 * i] = uint8_t(u >> 16);
    b[9 + 4 * i] = uint8_t(u >> 24);
  }
  persistentStore.access_start();
  persistentStore.write_data(SPOOL_EEPROM_START, b, 14);
  persistentStore.access_finish();
}

void SpoolTracker::task() {
  const bool job = printingIsActive() || printingIsPaused();

  if (job && !counting) {
    counting = true;
    extruded_mm = 0;
    job_type = pending_type;
    pending_type = SPOOL_NONE;
    pending_grams = 0;
    max_hotend_target = 0;
  }

  if (job) {
    const celsius_t t = thermalManager.degTargetHotend(0);
    if (t > max_hotend_target) max_hotend_target = t;
  }
  else if (counting) {
    counting = false;
    const int8_t t = job_type != SPOOL_NONE ? job_type : (max_hotend_target >= (SPOOL_PETG_MIN_TEMP) ? SPOOL_PETG : SPOOL_PLA);
    // mm of 1.75 mm filament -> mm3 -> cm3 -> grams
    const float grams = extruded_mm * 2.4053f * float(t == SPOOL_PETG ? SPOOL_DENSITY_PETG : SPOOL_DENSITY_PLA) / 1000.0f;
    if (grams > 0.5f) {
      const uint16_t g = uint16_t(grams + 0.5f);
      grams_used[t] += g;
      if (grams_left[t]) grams_left[t] = grams_left[t] > g ? grams_left[t] - g : 1; // 0 means "not set", 1 g = empty
      save();
    }
  }
}

void SpoolTracker::file_chosen(const char *abs_path) {
  SlicerInfo i;
  if (read_slicer_info(abs_path, i)) {
    pending_type = type_from_name(i.type);
    pending_grams = i.grams;
  }
  else {
    pending_type = SPOOL_NONE;
    pending_grams = 0;
  }
}

bool SpoolTracker::pending_low() {
  if (!pending_grams || pending_type == SPOOL_NONE) return false;
  const uint16_t left = grams_left[pending_type];
  return left && uint32_t(pending_grams) * 105UL / 100UL > left;
}

int8_t SpoolTracker::type_from_name(const char *s) {
  if (strncasecmp(s, "PLA", 3) == 0) return SPOOL_PLA;
  if (strncasecmp(s, "PET", 3) == 0) return SPOOL_PETG;
  return SPOOL_NONE;
}

void SpoolTracker::new_spool_pla()  { grams_left[SPOOL_PLA] = 1000;  save(); }
void SpoolTracker::new_spool_petg() { grams_left[SPOOL_PETG] = 1000; save(); }

void SpoolTracker::reset_used() {
  for (uint8_t i = 0; i < SPOOL_COUNT; i++) grams_used[i] = 0;
  save();
}

#endif // SPOOL_TRACKER
