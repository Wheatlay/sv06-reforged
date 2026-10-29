// Render a sheet of status-screen states to a PNG so the design can be checked without a browser.
// Usage: node tools/render_preview.js mockup/lcd_assets.json out.png [font_9x18Br]
const fs = require('fs');
const zlib = require('zlib');
const LCD = require('../mockup/lcd.js');

const A = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const big = process.argv[4] || 'font_9x18Br';
const cold = { hot: { cur: 24, tgt: 0 }, bed: { cur: 23, tgt: 0 }, fan: 0 };
const base = { night: true, hot: { cur: 240, tgt: 240 }, bed: { cur: 80, tgt: 80 }, fan: 60, pct: 83.7, left: '1:48',
  elapsed: '9:54', total: '11:49', file: 'plate_02_11h49m_228g.gcode', startsIn: '2:45', waitPct: 35, fonts: { big } };
const states = [
  { state: 'print' },
  { state: 'print', hot: { cur: 212, tgt: 240 }, pct: 2.4 },
  { state: 'print', paused: true, night: false },
  { state: 'print', msg: 'Filament change: press knob' },
  { state: 'wait', ...cold },
  { state: 'idle', ...cold, night: false },
  { state: 'idle', ...cold, night: false, msg: 'Probing bed...' },
  { state: 'idle', hot: { cur: 150, tgt: 240 }, bed: { cur: 60, tgt: 80 }, fan: 0, night: false, msg: 'Heating...' },
];
const shots = states.map((o) => LCD.drawStatus(A, { ...base, ...o }));

const cols = 2, rowsN = Math.ceil(shots.length / cols), gap = 4, k = 3;
const w = (cols * LCD.W + (cols + 1) * gap) * k, h = (rowsN * LCD.H + (rowsN + 1) * gap) * k;
const raw = Buffer.alloc((w * 3 + 1) * h);
for (let y = 0; y < h; y++) {
  raw[y * (w * 3 + 1)] = 0;
  for (let x = 0; x < w; x++) {
    const cx = Math.floor(x / k) - gap, cy = Math.floor(y / k) - gap;
    const ci = Math.floor(cx / (LCD.W + gap)), ri = Math.floor(cy / (LCD.H + gap));
    const lx = cx - ci * (LCD.W + gap), ly = cy - ri * (LCD.H + gap);
    const shot = cx >= 0 && cy >= 0 && ci < cols && lx < LCD.W && ly < LCD.H ? shots[ri * cols + ci] : null;
    const on = shot ? shot.px[ly * LCD.W + lx] : 2;
    const o = y * (w * 3 + 1) + 1 + x * 3;
    [raw[o], raw[o + 1], raw[o + 2]] = on === 2 ? [30, 32, 34] : on ? [232, 239, 255] : [26, 63, 208];
  }
}
const crc = (b) => { let c = ~0; for (const v of b) { c ^= v; for (let i = 0; i < 8; i++) c = (c >>> 1) ^ (0xedb88320 & -(c & 1)); } return ~c >>> 0; };
const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
fs.writeFileSync(process.argv[3], Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
