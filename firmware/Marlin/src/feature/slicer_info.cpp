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

#include "../inc/MarlinConfig.h"

#if ANY(DELAYED_START, SPOOL_TRACKER)

#include "slicer_info.h"
#include "../sd/cardreader.h"

#include <string.h>

static const char k_time[]  = "; estimated printing time (normal mode) =";
static const char k_grams[] = "; filament used [g] =";
static const char k_type[]  = "; filament_type =";

static void parse_line(const char *line, SlicerInfo &info) {
  if (!strncmp(line, k_time, sizeof(k_time) - 1)) {
    uint32_t secs = 0, num = 0;
    bool any = false;
    for (const char *c = line + sizeof(k_time) - 1; *c; ++c) {
      if (*c >= '0' && *c <= '9') { num = num * 10 + (*c - '0'); any = true; }
      else if (any && (*c == 'd' || *c == 'h' || *c == 'm' || *c == 's')) {
        secs += num * (*c == 'd' ? 86400UL : *c == 'h' ? 3600UL : *c == 'm' ? 60UL : 1UL);
        num = 0; any = false;
      }
      else if (*c != ' ') { num = 0; any = false; }
    }
    const uint32_t mins = (secs + 59) / 60;
    info.minutes = mins > 65535UL ? 65535 : uint16_t(mins);
  }
  else if (!strncmp(line, k_grams, sizeof(k_grams) - 1)) {
    const char *c = line + sizeof(k_grams) - 1;
    while (*c == ' ') ++c;
    uint32_t whole = 0;
    while (*c >= '0' && *c <= '9') { if (whole < 100000UL) whole = whole * 10 + (*c - '0'); ++c; }
    if (*c == '.' && c[1] >= '5' && c[1] <= '9') ++whole;  // round to nearest
    info.grams = whole > 65535UL ? 65535 : uint16_t(whole);
  }
  else if (!strncmp(line, k_type, sizeof(k_type) - 1)) {
    const char *c = line + sizeof(k_type) - 1;
    while (*c == ' ') ++c;
    uint8_t n = 0;
    while (*c && *c != ';' && *c != ' ' && *c != '\r' && *c != '\n' && n < sizeof(info.type) - 1) info.type[n++] = *c++;
    info.type[n] = '\0';
  }
}

bool read_slicer_info(const char *abs_path, SlicerInfo &info) {
  info.minutes = 0; info.grams = 0; info.type[0] = '\0';
  if (!card.isMounted()) return false;

  MediaFile *dir = nullptr;
  const char * const fname = card.diveToFile(false, dir, abs_path);
  if (!fname || !dir) return false;

  MediaFile f;
  if (!f.open(dir, fname, O_READ)) return false;

  const uint32_t size = f.fileSize();
  if (size > 20000UL) f.seekSet(size - 20000UL);

  char buf[128], line[96];
  uint8_t len = 0;
  for (;;) {
    const int16_t n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    for (int16_t i = 0; i < n; i++) {
      const char c = buf[i];
      if (c == '\n') { line[len] = '\0'; parse_line(line, info); len = 0; }
      else if (len < sizeof(line) - 1) line[len++] = c;  // overlong lines are truncated
    }
  }
  if (len) { line[len] = '\0'; parse_line(line, info); }
  f.close();
  return true;
}

#endif // DELAYED_START || SPOOL_TRACKER
