/**
 * SV06 Reforged (Marlin 3D Printer Firmware for the original Sovol SV06)
 * Copyright (c) 2026 Wheatlay
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
/**
 * SV06 custom 128x64 status screen (port of the JS mockup drawStatus in mockup/lcd.js).
 * Replaces the stock MarlinUI::draw_status_screen when SV06_STATUS_SCREEN is enabled.
 */

#include "../../inc/MarlinConfigPre.h"

#if BOTH(HAS_MARLINUI_U8GLIB, SV06_STATUS_SCREEN)

#include "marlinui_DOGM.h"
#include "../marlinui.h"
#include "../../module/temperature.h"
#include "../../module/printcounter.h"
#include "../../sd/cardreader.h"
#include "../../MarlinCore.h"
#include "../../gcode/gcode.h"
#include "status_screen_sv06_bitmaps.h"

#if ENABLED(NIGHT_MODE)
  #include "../../feature/night_mode.h"
#endif
#if ENABLED(DELAYED_START)
  #include "../../feature/delayed_start.h"
#endif
#if ENABLED(SPOOL_TRACKER)
  #include "../../feature/spool.h"
#endif

#define SV06_FONT_M u8g_font_6x10r
#define SV06_FONT_T u8g_font_4x6r
#define SV06_FONT_B u8g_font_9x18Br

namespace {

  // 3x3 degree ring
  void deg(const uint8_t x, const uint8_t y) {
    u8g.drawPixel(x + 1, y);
    u8g.drawPixel(x, y + 1);
    u8g.drawPixel(x + 2, y + 1);
    u8g.drawPixel(x + 1, y + 2);
  }

  void dotted_line(const uint8_t y) {
    for (uint8_t x = 0; x < 128; x += 2) u8g.drawPixel(x, y);
  }

  // Copy a flash string to RAM
  void flash_copy(char * const dst, const size_t size, const char * const src) {
    strncpy_P(dst, src, size - 1);
    dst[size - 1] = '\0';
  }

  void draw_right(const uint8_t xr, const uint8_t y, const char * const s) {
    u8g.drawStr(xr - u8g.getStrWidth(s), y, s);
  }

  void fmt_time(char * const buf, const uint32_t seconds, const bool unknown) {
    if (unknown) { strcpy_P(buf, PSTR("--:--")); return; }
    sprintf_P(buf, PSTR("%u:%02u"), uint16_t(seconds / 3600UL), uint16_t((seconds / 60UL) % 60UL));
  }

  // Heater "heating" state with hysteresis: starts when the target changes (or the heater drifts
  // more than 10 degrees off), ends only once the temperature reaches the target (within 1 degree)
  struct HeatWatch {
    celsius_t last_tgt = 0;
    bool active = false;
    bool update(const celsius_t cur, const celsius_t tgt) {
      const int diff = ABS(int(cur) - int(tgt));
      if (tgt <= 0) active = false;
      else if (tgt != last_tgt || diff > 10) active = diff > 1;
      else if (diff <= 1) active = false;
      last_tgt = tgt;
      return active;
    }
  };

  // Draw the bottom rows of a "heating" bitmap over the plain icon: power 127 = whole bitmap
  void fill_from_bottom(const uint8_t x, const uint8_t y, const uint8_t bw, const uint8_t h, const uint8_t *bmp, const int16_t power) {
    if (power <= 0) return;
    const uint8_t rows = _MIN(h, uint8_t((uint16_t(h) * power + 126) / 127));
    u8g.drawBitmapP(x, y + h - rows, bw, rows, bmp + (h - rows) * bw);
  }

  // set_font() caches the last font, but this screen also calls u8g.setFont directly: force a real switch
  void menu_font() { ui.set_font(FONT_STATUSMENU); ui.set_font(FONT_MENU); }

} // namespace

void MarlinUI::draw_status_screen() {
  enum State : uint8_t { ST_IDLE, ST_PRINT, ST_WAIT };

  static bool fan_frame = false;
  static char last_msg[25] = "";
  static millis_t msg_changed_ms = 0;

  if (first_page) {
    fan_frame = !fan_frame;
    if (strncmp(last_msg, status_message, 24)) {
      strncpy(last_msg, status_message, 24);
      last_msg[24] = '\0';
      msg_changed_ms = millis();
    }
  }

  const bool paused = printingIsPaused(),
             printing = printingIsActive() || paused,
             waiting = TERN0(DELAYED_START, delayed_start.armed()),
             night = TERN0(NIGHT_MODE, nightmode.enabled),
             blink = get_blink();
  const State state = printing ? ST_PRINT : waiting ? ST_WAIT : ST_IDLE;
  if (state != ST_PRINT) sv06_m73_seen = false; // next print starts without slicer values
  const bool has_msg = status_message[0] != '\0';
  const bool recent_msg = last_msg[0] && PENDING(millis(), msg_changed_ms + 20000UL);

  const celsius_t hot_cur = thermalManager.wholeDegHotend(0),
                  hot_tgt = thermalManager.degTargetHotend(0),
                  bed_cur = thermalManager.wholeDegBed(),
                  bed_tgt = thermalManager.degTargetBed();
  #if HAS_FAN
    const uint8_t fan = thermalManager.pwmToPercent(thermalManager.fan_speed[0]);
  #else
    const uint8_t fan = 0;
  #endif

  char buf[32];
  u8g.setColorIndex(1);

  // Top band
  u8g.setFont(SV06_FONT_M);

  // Icons fill from the bottom in proportion to the power the heater gets (0-127)
  u8g.drawBitmapP(0, 0, SV06_NOZZLE_BW, SV06_NOZZLE_H, sv06_nozzle_bmp);
  fill_from_bottom(0, 0, SV06_NOZZLE_ON_BW, SV06_NOZZLE_ON_H, sv06_nozzle_on_bmp, thermalManager.getHeaterPower(H_E0));
  sprintf_P(buf, PSTR("%u"), uint16_t(hot_cur));
  u8g.drawStr(12, 10, buf);
  deg(12 + u8g.getStrWidth(buf) + 1, 2);

  u8g.drawBitmapP(42, 5, SV06_BED_BW, SV06_BED_H, sv06_bed_bmp);
  fill_from_bottom(42, 0, SV06_BED_ON_BW, SV06_BED_ON_H, sv06_bed_on_bmp, thermalManager.getHeaterPower(H_BED));
  sprintf_P(buf, PSTR("%u"), uint16_t(bed_cur));
  u8g.drawStr(62, 10, buf);
  deg(62 + u8g.getStrWidth(buf) + 1, 2);

  if (fan > 0 && fan_frame) u8g.drawBitmapP(89, 0, SV06_FAN1_BW, SV06_FAN1_H, sv06_fan1_bmp);
  else                      u8g.drawBitmapP(89, 0, SV06_FAN0_BW, SV06_FAN0_H, sv06_fan0_bmp);
  sprintf_P(buf, PSTR("%u"), uint16_t(fan));
  u8g.drawStr(102, 10, buf);
  u8g.drawStr(102 + u8g.getStrWidth(buf), 10, "%");

  // Row under the band: both targets while either heater is heating, otherwise labels
  static HeatWatch hot_watch, bed_watch;
  static bool heating = false;
  if (first_page) {
    const bool h = hot_watch.update(hot_cur, hot_tgt), b = bed_watch.update(bed_cur, bed_tgt);
    heating = h || b;
  }
  if (heating) {
    menu_font();
    if (hot_tgt > 0) {
      sprintf_P(buf, PSTR("%u"), uint16_t(hot_tgt));
      u8g.drawStr(12, 21, buf);
      deg(12 + u8g.getStrWidth(buf) + 1, 15);
    }
    if (bed_tgt > 0) {
      sprintf_P(buf, PSTR("%u"), uint16_t(bed_tgt));
      u8g.drawStr(62, 21, buf);
      deg(62 + u8g.getStrWidth(buf) + 1, 15);
    }
    dotted_line(23);
  }
  else {
    if (state != ST_IDLE) {
      u8g.setFont(SV06_FONT_T);
      flash_copy(buf, sizeof(buf), state == ST_PRINT ? GET_TEXT(MSG_SV06_TIME_LEFT) : GET_TEXT(MSG_SV06_STARTS_IN));
      u8g.drawStr(0, 21, buf);
      flash_copy(buf, sizeof(buf), state == ST_PRINT ? GET_TEXT(MSG_SV06_ELAPSED) : GET_TEXT(MSG_SV06_PRINT_TAKES));
      draw_right(128, 21, buf);
    }
    dotted_line(13);
  }

  // Middle
  constexpr uint8_t BASE = 38;
  if (state == ST_PRINT) {
    u8g.setFont(SV06_FONT_B);
    // Orca's M73 R only; before the first M73 (start G-code, heating) the time is unknown
    const uint32_t left = sv06_m73_seen ? remaining_time : 0;
    fmt_time(buf, left, !sv06_m73_seen);
    u8g.drawStr(0, BASE, buf);
    fmt_time(buf, print_job_timer.duration(), false);
    draw_right(128, BASE, buf);
    if (night) u8g.drawBitmapP(56, 24, SV06_MOON_BW, SV06_MOON_H, sv06_moon_bmp);
  }
  else if (state == ST_WAIT) {
    #if ENABLED(DELAYED_START)
      u8g.setFont(SV06_FONT_B);
      fmt_time(buf, delayed_start.minutes_left() * 60UL, false);
      u8g.drawStr(0, BASE, buf);
      const uint16_t est = delayed_start.estimate_minutes();
      fmt_time(buf, est * 60UL, est == 0);
      draw_right(128, BASE, buf);
      if (night) u8g.drawBitmapP(56, 24, SV06_MOON_BW, SV06_MOON_H, sv06_moon_bmp);
    #endif
  }
  else if (has_msg) {
    menu_font();
    u8g.drawStr(0, 34, status_message);
  }
  else {
    u8g.setFont(SV06_FONT_B);
    flash_copy(buf, sizeof(buf), GET_TEXT(MSG_SV06_READY));
    u8g.drawStr(0, BASE, buf);
  }

  // Progress bar with the percentage inside it
  if (state != ST_IDLE) {
    uint16_t pm = 0;
    if (state == ST_PRINT)  // Orca's M73 P only; Marlin's file-position fallback shows ~2% during heating
      pm = sv06_m73_seen ? (progress_override & PROGRESS_MASK) : 0;
    #if ENABLED(DELAYED_START)
      else {
        const uint16_t armed = delayed_start.armed_minutes(), left = delayed_start.minutes_left();
        if (armed && left < armed) pm = uint32_t(10000UL) * (armed - left) / armed;
      }
    #endif
    if (pm > 10000) pm = 10000;

    // While paused the fill blinks; the label inversion follows the fill actually drawn
    const uint8_t fill = (paused && blink) ? 0 : (126UL * pm + 5000UL) / 10000UL;
    u8g.drawFrame(0, 42, 128, 10);
    if (fill) u8g.drawBox(1, 43, fill, 8);

    if (state == ST_WAIT)   flash_copy(buf, sizeof(buf), GET_TEXT(MSG_SV06_WAITING));
    else if (paused)        flash_copy(buf, sizeof(buf), GET_TEXT(MSG_SV06_PAUSED));
    else                    sprintf_P(buf, PSTR("%u%%"), uint16_t(pm / 100)); // Orca sends whole percents

    menu_font();
    int16_t x = (128 - u8g.getStrWidth(buf)) / 2;
    for (const char *c = buf; *c; ++c) {
      const char ch[2] = { *c, '\0' };
      const int16_t w = u8g.getStrWidth(ch);
      u8g.setColorIndex((x + w / 2) < (1 + fill) ? 0 : 1);
      u8g.drawStr(x, 50, ch);
      x += w;
    }
    u8g.setColorIndex(1);
  }
  #if ENABLED(SPOOL_TRACKER)
    else {
      // SV06: grams left per type, "--" = not set
      char pla[8] = "--", petg[8] = "--";
      if (spool.grams_left[SPOOL_PLA])  sprintf_P(pla, PSTR("%ug"), uint16_t(spool.grams_left[SPOOL_PLA]));
      if (spool.grams_left[SPOOL_PETG]) sprintf_P(petg, PSTR("%ug"), uint16_t(spool.grams_left[SPOOL_PETG]));
      sprintf_P(buf, PSTR("PLA %s  PETG %s"), pla, petg);
      menu_font();
      u8g.drawStr((128 - u8g.getStrWidth(buf)) / 2, 50, buf);
    }
  #endif

  // File name / message under a rule
  dotted_line(55);
  u8g.setFont(SV06_FONT_T);
  if (state != ST_IDLE && recent_msg) {
    u8g.drawBox(0, 56, 128, 8);
    u8g.setColorIndex(0);
    u8g.drawStr(1, 63, last_msg);
    u8g.setColorIndex(1);
  }
  else if (state == ST_IDLE) {
    flash_copy(buf, sizeof(buf), card.isMounted() ? GET_TEXT(MSG_SV06_CHOOSE_FILE) : GET_TEXT(MSG_SV06_NO_FILE));
    u8g.drawStr(0, 63, buf);
  }
  else if (state == ST_PRINT) {
    if (card.isFileOpen()) {
      // 32 chars fit in 4x6; longer names scroll (2 s pause at each end, 3 chars/s)
      const char * const name = card.longest_filename();
      const uint8_t len = strlen(name);
      uint8_t ofs = 0;
      if (len > 32) {
        const uint8_t span = len - 32, cycle = span + 12;
        const uint8_t t = (millis() / 333UL) % (2 * cycle);
        const uint8_t pos = t < cycle ? t : 2 * cycle - 1 - t;   // forward, then back
        ofs = pos < 6 ? 0 : _MIN(uint8_t(pos - 6), span);
      }
      u8g.drawStr(0, 63, name + ofs);
    }
  }
  else {
    #if ENABLED(DELAYED_START)
      u8g.drawStr(0, 63, delayed_start.filename());
    #endif
  }

  menu_font(); // leave the real menu font set, so menus don't inherit 4x6
}

#endif // HAS_MARLINUI_U8GLIB && SV06_STATUS_SCREEN
