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

#include "../inc/MarlinConfigPre.h"

#if ENABLED(PRINTED_MARKS)

#include "printed_marks.h"
#include "../sd/cardreader.h"
#include "../HAL/shared/eeprom_api.h"

#include <string.h>

PrintedMarks printed_marks;

PrintedMarks::Entry PrintedMarks::entries[PRINTED_MARKS_MAX];
uint8_t PrintedMarks::used;           // = 0
uint32_t PrintedMarks::current_hash;  // = 0

// Stored in the EEPROM above the settings (V88 settings end near byte 825 of 2048), never on the card:
// a PRINTED.TXT in the card root left the Sovol bootloader stuck on a blue screen (29.10.2026).
// Layout: 'P' 'M' <used> then <hash 4 bytes> <count 1 byte> per entry.
#define PM_EEPROM_START 1536
static_assert(PM_EEPROM_START + 3 + 5 * (PRINTED_MARKS_MAX) <= MARLIN_EEPROM_SIZE, "PRINTED_MARKS_MAX too big for the EEPROM");

// 32-bit FNV-1a (never returns 0)
uint32_t PrintedMarks::hash(const char *s) {
  uint32_t h = 2166136261UL;
  while (*s) { h ^= (uint8_t)*s++; h *= 16777619UL; }
  return h ? h : 1;
}

void PrintedMarks::load() {
  used = 0;
  uint8_t head[3];
  if (persistentStore.read_data(PM_EEPROM_START, head, 3)) return;
  if (head[0] != 'P' || head[1] != 'M' || head[2] > PRINTED_MARKS_MAX) return; // never saved yet
  for (uint8_t i = 0; i < head[2]; i++) {
    uint8_t b[5];
    if (persistentStore.read_data(PM_EEPROM_START + 3 + 5 * i, b, 5)) return;
    const uint32_t h = uint32_t(b[0]) | uint32_t(b[1]) << 8 | uint32_t(b[2]) << 16 | uint32_t(b[3]) << 24;
    if (h && b[4]) entries[used++] = { h, b[4] };
  }
}

void PrintedMarks::print_started() {
  current_hash = hash(card.longest_filename());
}

void PrintedMarks::print_finished() {
  if (!current_hash) return;

  Entry e = { current_hash, 0 };
  uint8_t idx = used;
  for (uint8_t i = 0; i < used; i++) if (entries[i].hash == current_hash) { idx = i; break; }

  if (idx < used) {
    e.count = entries[idx].count;
    for (uint8_t i = idx; i < used - 1; i++) entries[i] = entries[i + 1]; // remove, re-append at end
    used--;
  }
  if (e.count < 99) e.count++;

  if (used >= PRINTED_MARKS_MAX) { // Full: drop the oldest
    for (uint8_t i = 0; i < used - 1; i++) entries[i] = entries[i + 1];
    used--;
  }
  entries[used++] = e;

  current_hash = 0;
  save();
}

uint8_t PrintedMarks::count_for(const char *longname) {
  if (!used) return 0;
  const uint32_t h = hash(longname);
  for (uint8_t i = 0; i < used; i++) if (entries[i].hash == h) return entries[i].count;
  return 0;
}

void PrintedMarks::save() {
  // Only changed bytes are written (a few ms each), so a save after a print takes well under a second
  persistentStore.access_start();
  for (uint8_t i = 0; i < used; i++) {
    const uint32_t h = entries[i].hash;
    const uint8_t b[5] = { uint8_t(h), uint8_t(h >> 8), uint8_t(h >> 16), uint8_t(h >> 24), entries[i].count };
    persistentStore.write_data(PM_EEPROM_START + 3 + 5 * i, b, 5);
  }
  const uint8_t head[3] = { 'P', 'M', used };
  persistentStore.write_data(PM_EEPROM_START, head, 3);
  persistentStore.access_finish();
}

#endif // PRINTED_MARKS
