// Render README screenshots (status screen while printing, idle main menu) with the firmware's own fonts and
// bitmaps, following Marlin/src/lcd/dogm/status_screen_sv06.cpp and the DOGM menu layout.
// Usage: node tools/render_readme_shots.js <out_dir>
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const LCD = require('../mockup/lcd.js');

const ROOT = path.join(__dirname, '..');
const A = JSON.parse(fs.readFileSync(path.join(ROOT, 'mockup/lcd_assets.json'), 'utf8'));
const F = { M: A.font_6x10r, T: A.font_tiny, B: A.font_9x18Br, MENU: A.font_menu };

// Bitmaps from the firmware header: "#define SV06_<NAME>_H n" followed by rows of B........ bytes
const hdr = fs.readFileSync(path.join(ROOT, 'firmware/Marlin/src/lcd/dogm/status_screen_sv06_bitmaps.h'), 'utf8');
const BMP = {};
for (const m of hdr.matchAll(/#define SV06_(\w+)_W (\d+)[\s\S]*?\{([\s\S]*?)\};/g)) {
  const w = +m[2];
  BMP[m[1]] = m[3].split('\n').map(l => (l.match(/B[01]{8}/g) || []).map(b => b.slice(1)).join('').slice(0, w)).filter(r => r);
}

const deg = (s, x, y) => { for (const [dx, dy] of [[1, 0], [0, 1], [2, 1], [1, 2]]) LCD.box(s, x + dx, y + dy, 1, 1); };
const dotted = (s, y) => { for (let x = 0; x < 128; x += 2) LCD.box(s, x, y, 1, 1); };
const right = (s, f, xr, y, t) => LCD.text(s, f, xr - LCD.width(f, t), y, t);
const fill = (s, x, y, rows, power) => {   // fill_from_bottom()
  if (power <= 0) return;
  const n = Math.min(rows.length, Math.floor((rows.length * power + 126) / 127));
  LCD.bitmap(s, x, y + rows.length - n, rows.slice(rows.length - n));
};

function status(o) {
  const s = LCD.screen();
  LCD.bitmap(s, 0, 0, BMP.NOZZLE); fill(s, 0, 0, BMP.NOZZLE_ON, o.hotPower);
  let t = String(o.hot); LCD.text(s, F.M, 12, 10, t); deg(s, 12 + LCD.width(F.M, t) + 1, 2);
  LCD.bitmap(s, 42, 5, BMP.BED); fill(s, 42, 0, BMP.BED_ON, o.bedPower);
  t = String(o.bed); LCD.text(s, F.M, 62, 10, t); deg(s, 62 + LCD.width(F.M, t) + 1, 2);
  LCD.bitmap(s, 89, 0, o.fan ? BMP.FAN1 : BMP.FAN0);
  t = String(o.fan); LCD.text(s, F.M, 102, 10, t); LCD.text(s, F.M, 102 + LCD.width(F.M, t), 10, '%');

  LCD.text(s, F.T, 0, 21, 'TIME LEFT'); right(s, F.T, 128, 21, 'ELAPSED'); dotted(s, 13);
  LCD.text(s, F.B, 0, 38, o.left); right(s, F.B, 128, 38, o.elapsed);
  if (o.night) LCD.bitmap(s, 56, 24, BMP.MOON);

  const f = Math.floor((126 * o.pct * 100 + 5000) / 10000);
  LCD.frame(s, 0, 42, 128, 10); LCD.box(s, 1, 43, f, 8);
  const label = `${o.pct}%`;
  let x = Math.floor((128 - LCD.width(F.MENU, label)) / 2);
  for (const ch of label) { const w = LCD.width(F.MENU, ch); LCD.text(s, F.MENU, x, 50, ch, (x + w / 2) < (1 + f) ? 0 : 1); x += w; }

  dotted(s, 55); LCD.text(s, F.T, 0, 63, o.file.slice(0, 32));
  return s;
}

function menu(items, sel) {
  const s = LCD.screen();
  items.forEach((it, row) => {
    const y1 = row * 12 + 1, y2 = y1 + 11;
    if (row === sel) { LCD.box(s, 0, y1 + 1, 128, 1); LCD.box(s, 0, y2 + 2, 128, 1); }
    LCD.text(s, F.MENU, 0, y2, it.label);
    if (it.sub) LCD.text(s, F.MENU, 128 - 6, y2, String.fromCharCode(3));
  });
  return s;
}

function png(file, s, k = 4, pad = 3) {
  const w = (128 + 2 * pad) * k, h = (64 + 2 * pad) * k, raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const lx = Math.floor(x / k) - pad, ly = Math.floor(y / k) - pad;
    const on = lx >= 0 && ly >= 0 && lx < 128 && ly < 64 ? s.px[ly * 128 + lx] : 0;
    const gap = false;
    const o = y * (w * 3 + 1) + 1 + x * 3;
    [raw[o], raw[o + 1], raw[o + 2]] = on && !gap ? [232, 239, 255] : [26, 63, 208];
  }
  const crc = (b) => { let c = ~0; for (const v of b) { c ^= v; for (let i = 0; i < 8; i++) c = (c >>> 1) ^ (0xedb88320 & -(c & 1)); } return ~c >>> 0; };
  const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
  fs.writeFileSync(file, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
}

const out = process.argv[2] || '.';
png(path.join(out, 'status-screen.png'), status({
  hot: 210, hotPower: 64, bed: 60, bedPower: 90, fan: 100, left: '1:24', elapsed: '0:41', pct: 33,
  night: false, file: 'benchy_PLA_0.2mm_2h05m.gcode',
}));
png(path.join(out, 'main-menu.png'), menu([
  { label: 'Info Screen' }, { label: 'Print from Media', sub: true }, { label: 'Level Bed' },
  { label: 'Change Filament', sub: true }, { label: 'Spools', sub: true },
], 1));
