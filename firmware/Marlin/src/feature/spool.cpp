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
#include "../HAL/shared/eeprom_api.h"

#include <string.h>
#include <strings.h>

SpoolTracker spool;

Spool SpoolTracker::spools[SPOOL_MAX];   // = all empty
uint8_t SpoolTracker::active;            // = 0
bool SpoolTracker::counting;             // = false
float SpoolTracker::extruded_mm;         // = 0
uint16_t SpoolTracker::pending_grams;    // = 0
char SpoolTracker::pending_type[8];      // = ""

// Printed marks use 1536..1778 (1536 + 3 + 5*48), settings end near 825.
// Layout: 'S' '2', active, then 8 records of 18 bytes:
//   name[10] (no terminator, padded with 0), density_cg, grams_left (u16 LE), grams_used (u32 LE), carry_cg.
// The older layout was 'S' 'P', grams_left[2] u16 LE, grams_used[2] u32 LE, carry_cg[2] (16 bytes): PLA, PETG.
#define SPOOL_EEPROM_START 1800
#define SPOOL_REC_SIZE     18
#define SPOOL_EEPROM_SIZE  (3 + (SPOOL_MAX) * (SPOOL_REC_SIZE))
static_assert(SPOOL_EEPROM_START + SPOOL_EEPROM_SIZE <= MARLIN_EEPROM_SIZE, "Spool tracker data does not fit in the EEPROM");
#if ENABLED(PRINTED_MARKS)
  static_assert(SPOOL_EEPROM_START >= 1536 + 3 + 5 * (PRINTED_MARKS_MAX), "Spool tracker data overlaps the printed marks");
#endif

static void spool_defaults() {
  memset(SpoolTracker::spools, 0, sizeof(SpoolTracker::spools));
  strcpy(SpoolTracker::spools[0].name, "PLA");
  SpoolTracker::spools[0].density_cg = 124;
  strcpy(SpoolTracker::spools[1].name, "PETG");
  SpoolTracker::spools[1].density_cg = 127;
  SpoolTracker::active = 0;
}

void SpoolTracker::load() {
  uint8_t b[SPOOL_EEPROM_SIZE];
  spool_defaults();
  if (persistentStore.read_data(SPOOL_EEPROM_START, b, 16)) return;

  if (b[0] == 'S' && b[1] == 'P') { // old PLA / PETG layout: migrate
    for (uint8_t i = 0; i < 2; i++) {
      Spool &s = spools[i];
      s.grams_left = uint16_t(b[2 + 2 * i]) | uint16_t(b[3 + 2 * i]) << 8;
      const uint8_t *u = b + 6 + 4 * i;
      s.grams_used = uint32_t(u[0]) | uint32_t(u[1]) << 8 | uint32_t(u[2]) << 16 | uint32_t(u[3]) << 24;
      s.carry_cg = b[14 + i] < 100 ? b[14 + i] : 0; // older 14-byte data: unwritten bytes read as 0xFF
    }
    save();
    return;
  }

  if (b[0] != 'S' || b[1] != '2') return; // never saved yet
  if (persistentStore.read_data(SPOOL_EEPROM_START, b, SPOOL_EEPROM_SIZE)) { spool_defaults(); return; }

  active = b[2];
  for (uint8_t i = 0; i < SPOOL_MAX; i++) {
    const uint8_t *r = b + 3 + i * SPOOL_REC_SIZE;
    Spool &s = spools[i];
    memset(&s, 0, sizeof(Spool));
    for (uint8_t n = 0; n < SPOOL_NAME_LEN; n++) {
      const char c = char(r[n]);
      if (c < 32 || c > 126) break; // end of name (0 padding, or garbage)
      s.name[n] = c;
    }
    if (!s.name[0]) continue; // empty slot
    s.density_cg = (r[10] >= 50 && r[10] <= 250) ? r[10] : 124;
    s.grams_left = uint16_t(r[11]) | uint16_t(r[12]) << 8;
    s.grams_used = uint32_t(r[13]) | uint32_t(r[14]) << 8 | uint32_t(r[15]) << 16 | uint32_t(r[16]) << 24;
    s.carry_cg = r[17] < 100 ? r[17] : 0;
  }

  if (active >= SPOOL_MAX || !spools[active].name[0]) {
    uint8_t i = 0;
    while (i < SPOOL_MAX && !spools[i].name[0]) i++;
    if (i == SPOOL_MAX) spool_defaults(); // everything empty: start over
    else active = i;
  }
}

void SpoolTracker::save() {
  uint8_t b[SPOOL_EEPROM_SIZE] = { 'S', '2', active };
  for (uint8_t i = 0; i < SPOOL_MAX; i++) {
    uint8_t *r = b + 3 + i * SPOOL_REC_SIZE;
    const Spool &s = spools[i];
    memset(r, 0, SPOOL_REC_SIZE);
    memcpy(r, s.name, strnlen(s.name, SPOOL_NAME_LEN));
    r[10] = s.density_cg;
    r[11] = uint8_t(s.grams_left);
    r[12] = uint8_t(s.grams_left >> 8);
    r[13] = uint8_t(s.grams_used);
    r[14] = uint8_t(s.grams_used >> 8);
    r[15] = uint8_t(s.grams_used >> 16);
    r[16] = uint8_t(s.grams_used >> 24);
    r[17] = s.carry_cg;
  }
  persistentStore.access_start();
  persistentStore.write_data(SPOOL_EEPROM_START, b, SPOOL_EEPROM_SIZE);
  persistentStore.access_finish();
}

void SpoolTracker::task() {
  const bool job = printingIsActive() || printingIsPaused();

  if (job && !counting) {
    counting = true;
    extruded_mm = 0;
    pending_type[0] = '\0';
    pending_grams = 0;
  }
  else if (!job && counting) {
    counting = false;
    Spool &s = cur();
    // mm of 1.75 mm filament -> mm3 -> cm3 -> grams
    const float grams = extruded_mm * 2.4053f * float(s.density_cg) / 100.0f / 1000.0f;
    // Whole grams are counted now; the fraction carries over to the next print, so small prints add up
    const int32_t cg = int32_t(grams * 100.0f + 0.5f) + s.carry_cg;
    if (cg > 0) {
      const uint16_t g = uint16_t(cg / 100);
      s.carry_cg = uint8_t(cg % 100);
      if (g) {
        s.grams_used += g;
        if (s.grams_left) s.grams_left = s.grams_left > g ? s.grams_left - g : 1; // 0 means "not set", 1 g = empty
      }
      save();
    }
  }
}

void SpoolTracker::file_chosen(const char *abs_path) {
  SlicerInfo i;
  if (read_slicer_info(abs_path, i)) {
    pending_grams = i.grams;
    strlcpy(pending_type, i.type, sizeof(pending_type));
  }
  else {
    pending_grams = 0;
    pending_type[0] = '\0';
  }
}

bool SpoolTracker::pending_low() {
  if (!pending_grams) return false;
  const uint16_t left = cur().grams_left;
  return left && uint32_t(pending_grams) * 105UL / 100UL > left;
}

bool SpoolTracker::pending_mismatch() {
  const size_t n = strlen(pending_type);
  return n && strncasecmp(cur().name, pending_type, n) != 0;
}

void SpoolTracker::new_spool_1kg() { cur().grams_left = 1000; save(); }

void SpoolTracker::reset_used() {
  cur().grams_used = 0;
  cur().carry_cg = 0;
  save();
}

void SpoolTracker::select(uint8_t i) {
  if (i < SPOOL_MAX && spools[i].name[0]) { active = i; save(); }
}

#endif // SPOOL_TRACKER
