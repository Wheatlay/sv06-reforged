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
 * Delayed start: start a selected SD file after a countdown ("start in 2:30"), like a dishwasher timer.
 * Nothing heats or moves while waiting. Cancelled from the main menu, or automatically if the card is gone.
 */

#include "../inc/MarlinConfig.h"
#include "../sd/cardreader.h"

class DelayedStart {
public:
  static void arm(const uint16_t minutes);   // Use the file currently selected in the card browser
  static void cancel();
  static bool armed() { return deadline != 0; }
  static uint16_t minutes_left();
  static void task();                        // Called from idle()
  static uint16_t estimate_minutes() { return estimate_min; }  // Print time from the G-code header, 0 = unknown
  static uint16_t armed_minutes() { return armed_min; }        // Delay chosen at arm()
  static const char* filename();             // Name of the armed file (without directories)

private:
  static uint16_t estimate_min, armed_min;
  static millis_t deadline;                  // 0 = not armed
  static char path[MAXPATHNAMELENGTH];       // Root-relative DOS path of the file to print
  static void show_status();
};

extern DelayedStart delayed_start;
