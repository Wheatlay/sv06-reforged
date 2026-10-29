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

#include "../inc/MarlinConfigPre.h"

#if ENABLED(NIGHT_MODE)

#include "night_mode.h"
#include "../lcd/marlinui.h"

NightMode nightmode;

bool NightMode::enabled; // = false

void NightMode::changed() {
  ui.set_status(enabled ? GET_TEXT_F(MSG_NIGHT_MODE_ON) : GET_TEXT_F(MSG_NIGHT_MODE_OFF));
}

#endif // NIGHT_MODE
