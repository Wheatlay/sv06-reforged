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

#if ENABLED(DELAYED_START)

#include "delayed_start.h"
#include "slicer_info.h"
#include "../sd/cardreader.h"
#include "../lcd/marlinui.h"
#include "../MarlinCore.h"

DelayedStart delayed_start;

millis_t DelayedStart::deadline; // = 0
char DelayedStart::path[MAXPATHNAMELENGTH];

uint16_t DelayedStart::estimate_min; // = 0
uint16_t DelayedStart::armed_min;    // = 0

const char* DelayedStart::filename() {
  const char * const s = strrchr(path, '/');
  return s ? s + 1 : path;
}

void DelayedStart::arm(const uint16_t minutes) {
  card.getAbsSelectedFilename(path);
  if (!path[0]) return;  // Path too long
  armed_min = minutes;
  SlicerInfo info;
  estimate_min = read_slicer_info(path, info) ? info.minutes : 0;
  deadline = millis() + minutes * 60000UL;
  if (!deadline) deadline = 1;  // 0 means "not armed"
  show_status();
}

void DelayedStart::cancel() {
  deadline = 0;
  ui.set_status(GET_TEXT_F(MSG_DELAYED_START_CANCELLED));
}

uint16_t DelayedStart::minutes_left() {
  if (!armed()) return 0;
  const millis_t now = millis();
  return ELAPSED(now, deadline) ? 0 : (deadline - now + 59999UL) / 60000UL;
}

void DelayedStart::show_status() {
  const uint16_t m = minutes_left();
  ui.status_printf(0, F(S_FMT " %u:%02u"), GET_TEXT(MSG_DELAYED_START_IN), m / 60, m % 60);
}

void DelayedStart::task() {
  if (!armed()) return;

  static millis_t next_status_ms = 0;
  const millis_t now = millis();

  if (ELAPSED(now, deadline)) {
    deadline = 0;
    if (!card.isMounted() || printingIsActive())
      ui.set_status(GET_TEXT_F(MSG_DELAYED_START_FAILED));
    else
      card.openAndPrintFile(path);
    return;
  }

  if (ELAPSED(now, next_status_ms)) {  // Refresh the countdown once a minute
    next_status_ms = now + 60000UL;
    show_status();
  }
}

#endif // DELAYED_START
