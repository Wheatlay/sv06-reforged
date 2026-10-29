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

#include "../../../inc/MarlinConfig.h"

#if ENABLED(SPOOL_TRACKER)

#include "../../gcode.h"
#include "../../../MarlinCore.h"
#include "../../../feature/spool.h"

#include <string.h>

/**
 * M1100: SV06 spool tracker. List, define, edit, delete and load spools (slots 0..7).
 *
 *  S<n>        Slot 0..7
 *  N<name>     Name, max 10 chars; N"Name with spaces" for spaces. Creates the slot or renames it.
 *  T<material> PLA, PETG, ABS, ASA, TPU or OTHER (case-insensitive). New slot: default from the name.
 *              Without D the density follows the material.
 *  D<float>    Density in g/cm3 (0.50..2.50)
 *  G<int>      Grams left (0 = not set, 1 = empty)
 *  U<int>      Grams used (total)
 *  X           Delete slot S (not the last spool)
 *  A<n>        Load slot n (the spool that extrusion is counted against)
 *
 * Without parameters the spools are listed: S<n> "<name>" T<material> D<density> G<left> U<used>, * = loaded.
 *
 * Examples:
 *   M1100                          ; list spools
 *   M1100 S2 N"Red Silk" TPLA G1000 ; define slot 2
 *   M1100 S2 G640                  ; set grams left
 *   M1100 A2                       ; load slot 2
 *   M1100 S2 X                     ; delete slot 2
 */

struct M1100Args {
  bool has_s, has_n, has_d, has_g, has_u, has_x, has_a, has_t;
  uint8_t s, a, density_cg, material;
  uint16_t grams;
  uint32_t used;
  char name[SPOOL_NAME_LEN + 1];
};

// BEGIN-PARSE (kept free of Marlin types so it can be tested on the host)
// Returns nullptr if OK, else an error text.
static const char* m1100_parse(const char *p, M1100Args &r) {
  memset(&r, 0, sizeof(r));
  auto up = [](char c) { return (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c; };
  auto isdig = [](char c) { return c >= '0' && c <= '9'; };
  if (!p) return nullptr;
  while (*p) {
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;
    const char c = up(*p++);
    switch (c) {
      case 'S': case 'A': {
        if (!isdig(*p)) return "M1100: slot number expected";
        uint32_t v = 0;
        while (isdig(*p)) { v = v * 10 + uint32_t(*p++ - '0'); if (v > 99) v = 99; }
        if (v >= SPOOL_MAX) return "M1100: slot must be 0..7";
        if (c == 'S') { r.has_s = true; r.s = uint8_t(v); } else { r.has_a = true; r.a = uint8_t(v); }
      } break;
      case 'N': {
        const bool quoted = (*p == '"');
        if (quoted) p++;
        uint8_t n = 0;
        while (*p && (quoted ? *p != '"' : (*p != ' ' && *p != '\t'))) {
          if (n < SPOOL_NAME_LEN) r.name[n++] = *p;
          p++;
        }
        if (quoted) { if (*p != '"') return "M1100: missing closing quote"; p++; }
        r.name[n] = '\0';
        if (!n) return "M1100: empty name";
        r.has_n = true;
      } break;
      case 'T': {
        char tok[8];
        uint8_t n = 0;
        while (*p && *p != ' ' && *p != '\t') { if (n < sizeof(tok) - 1) tok[n++] = *p; p++; }
        tok[n] = '\0';
        const uint8_t m = n ? material_from_name(tok) : 0xFF;
        if (m == 0xFF) return "M1100: unknown material";
        r.has_t = true; r.material = m;
      } break;
      case 'D': {
        if (!isdig(*p) && *p != '.') return "M1100: density expected";
        uint32_t whole = 0, frac = 0, scale = 1;
        while (isdig(*p)) { if (whole < 1000) whole = whole * 10 + uint32_t(*p - '0'); p++; }
        if (*p == '.') {
          p++;
          while (isdig(*p)) { if (scale < 100) { frac = frac * 10 + uint32_t(*p - '0'); scale *= 10; } p++; }
        }
        if (scale == 1) { frac = 0; } else if (scale == 10) { frac *= 10; }
        const uint32_t cg = whole * 100 + frac;
        if (cg < 50 || cg > 250) return "M1100: density must be 0.50..2.50";
        r.has_d = true; r.density_cg = uint8_t(cg);
      } break;
      case 'G': case 'U': {
        if (!isdig(*p)) return "M1100: number expected";
        uint32_t v = 0;
        while (isdig(*p)) { v = v * 10 + uint32_t(*p++ - '0'); if (v > 4000000000UL / 10 - 10) v = 4000000000UL / 10 - 10; }
        if (c == 'G') {
          if (v > 65535) return "M1100: grams must be 0..65535";
          r.has_g = true; r.grams = uint16_t(v);
        }
        else { r.has_u = true; r.used = v; }
      } break;
      case 'X': r.has_x = true; break;
      default: return "M1100: unknown parameter";
    }
    if (*p && *p != ' ' && *p != '\t') return "M1100: parameters must be separated by spaces";
  }
  return nullptr;
}
// END-PARSE

static void m1100_list() {
  for (uint8_t i = 0; i < SPOOL_MAX; i++) {
    const Spool &s = spool.spools[i];
    if (!s.name[0]) continue;
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM("S", i, " \"", s.name, "\" T", material_name(s.material), " D", s.density_cg / 100, ".");
    if (s.density_cg % 100 < 10) SERIAL_CHAR('0');
    SERIAL_ECHO(s.density_cg % 100);
    SERIAL_ECHOPGM(" G", s.grams_left, " U", s.grams_used);
    if (i == spool.active) SERIAL_ECHOPGM(" *");
    SERIAL_EOL();
  }
}

void GcodeSuite::M1100() {
  M1100Args a;
  const char * const err = m1100_parse(parser.string_arg, a);
  if (err) { SERIAL_ERROR_START(); SERIAL_ECHOLN(err); return; }

  bool changed = false;

  // Editing during a print would pause it for the EEPROM write; loading a spool (A) is one byte, so it's allowed
  // (lets a slicer filament profile's start G-code pick the spool)
  if (a.has_s && (printingIsActive() || printingIsPaused())) { SERIAL_ERROR_MSG("M1100: can't edit spools while printing"); return; }

  if (a.has_s) {
    Spool &s = spool.spools[a.s];
    if (a.has_x) {
      uint8_t others = 0;
      for (uint8_t i = 0; i < SPOOL_MAX; i++) if (i != a.s && spool.spools[i].name[0]) others++;
      if (!s.name[0]) { SERIAL_ERROR_MSG("M1100: slot empty"); return; }
      if (!others) { SERIAL_ERROR_MSG("M1100: can't delete the last spool"); return; }
      memset(&s, 0, sizeof(Spool));
      if (spool.active == a.s) {
        uint8_t i = 0;
        while (!spool.spools[i].name[0]) i++;
        spool.active = i;
      }
      changed = true;
    }
    else if (a.has_n || s.name[0]) {
      const bool is_new = !s.name[0];
      if (a.has_n) strcpy(s.name, a.name);
      if (is_new) {
        const uint8_t m = a.has_t ? a.material : material_from_name(s.name);
        s.material = m == 0xFF ? uint8_t(MAT_OTHER) : m;
        s.density_cg = material_density_cg(s.material);
      }
      else if (a.has_t) {
        s.material = a.material;
        if (!a.has_d) s.density_cg = material_density_cg(a.material);
      }
      if (a.has_d) s.density_cg = a.density_cg;
      if (a.has_g) s.grams_left = a.grams;
      if (a.has_u) { s.grams_used = a.used; s.carry_cg = 0; }
      changed = true;
    }
    else { SERIAL_ERROR_MSG("M1100: slot empty, give N"); return; }
  }
  else if (a.has_n || a.has_t || a.has_d || a.has_g || a.has_u || a.has_x) {
    SERIAL_ERROR_MSG("M1100: give S<slot>");
    return;
  }

  if (a.has_a) {
    if (!spool.spools[a.a].name[0]) { SERIAL_ERROR_MSG("M1100: slot empty"); if (changed) spool.save(); return; }
    spool.active = a.a;
    changed = true;
  }

  if (changed) spool.save();
  m1100_list();
}

#endif // SPOOL_TRACKER
