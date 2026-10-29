// 128x64 status screen renderer using fonts/bitmaps extracted from Marlin/U8glib (see tools/extract_lcd_assets.py).
// Works in the browser (global LCD) and in Node (module.exports).
(function (root) {
  const W = 128, H = 64;

  function screen() { return { px: new Uint8Array(W * H) }; }
  function set(s, x, y, v = 1) { if (x >= 0 && x < W && y >= 0 && y < H) s.px[y * W + x] = v === 'x' ? s.px[y * W + x] ^ 1 : v; }

  function bitmap(s, x, y, rows) {
    rows.forEach((r, j) => { for (let i = 0; i < r.length; i++) if (r[i] === '1') set(s, x + i, y + j); });
  }
  function box(s, x, y, w, h, v = 1) { for (let j = 0; j < h; j++) for (let i = 0; i < w; i++) set(s, x + i, y + j, v); }
  function hline(s, x, y, w, dotted) { for (let i = 0; i < w; i++) if (!dotted || i % 2 === 0) set(s, x + i, y); }
  function frame(s, x, y, w, h) {
    for (let i = 0; i < w; i++) { set(s, x + i, y); set(s, x + i, y + h - 1); }
    for (let j = 0; j < h; j++) { set(s, x, y + j); set(s, x + w - 1, y + j); }
  }
  const glyph = (font, ch) => font.glyphs[ch.codePointAt(0)] || font.glyphs[63];
  // u8g text at (x, baseline). v='x' draws inverted over whatever is below. Returns the end x.
  function text(s, font, x, base, str, v = 1) {
    for (const ch of str) {
      const g = glyph(font, ch);
      if (!g) continue;
      const top = base - (g.h + g.y);
      g.rows.forEach((r, j) => { for (let i = 0; i < r.length; i++) if (r[i] === '1') set(s, x + g.x + i, top + j, v); });
      x += g.dx;
    }
    return x;
  }
  function width(font, str) { let w = 0; for (const ch of str) { const g = glyph(font, ch); if (g) w += g.dx; } return w; }
  const right = (s, font, xr, base, str, v) => text(s, font, xr - width(font, str), base, str, v);

  const DEG = String.fromCharCode(9);
  // 9x9 icons for the top band
  const ICON = {
    nozzle: ['011111110', '010000010', '010000010', '011111110', '001000100', '000101000', '000010000', '000000000', '000000000'],
    nozzleOn: ['011111110', '011111110', '011111110', '011111110', '001111100', '000111000', '000010000', '000000000', '000000000'],
    bed: ['000000000', '000000000', '000000000', '000000000', '000000000', '111111111', '100000001', '111111111', '000000000'],
    bedOn: ['010010010', '001001001', '010010010', '001001001', '000000000', '111111111', '111111111', '111111111', '000000000'],
    fan: ['000111000', '001111000', '001100011', '000101111', '011010110', '111101000', '110001100', '000111100', '000111000'],
    moon: ['001110000', '011000000', '110000000', '110000000', '110000000', '110000001', '011000010', '001111100', '000000000'],
  };

  // Stock Marlin icons trimmed to fit the 12px top band
  const dropCols = (rows, every) => rows.map(r => [...r].filter((_, i) => (i + 1) % every).join(''));
  function stockIcons(A) {
    if (A._icons) return A._icons;
    const crop = (rows) => rows.map(r => r.slice(2, 12));
    A._icons = {
      nozzle: crop(A.hotend_off.slice(0, 11)), nozzleOn: crop([A.hotend_off[0], ...A.hotend_off.slice(11)]),
      bed: dropCols(A.bed_off, 4), bedOn: dropCols(A.bed_on, 4),
      // Stock fan is 20x18 and does not shrink cleanly; same design redrawn at 11x11, two animation frames
      fan: [
        ['00011111000', '01100000110', '01001110010', '10000100001', '10100000101', '10110101101',
          '10100000101', '10000100001', '01001110010', '01100000110', '00011111000'],
        ['00011111000', '01100000110', '01011011010', '10011011001', '10000000001', '10000100001',
          '10000000001', '10011011001', '01011011010', '01100000110', '00011111000'],
      ],
    };
    return A._icons;
  }
  // 16x16 moon for the middle gap
  const MOON16 = ['0000011111000000', '0001111100000000', '0011110000000000', '0111100000000000', '0111000000000000', '1111000000000000',
    '1110000000000000', '1110000000000000', '1110000000000000', '1111000000000001', '0111000000000010', '0111100000000110',
    '0011111000011100', '0001111111111000', '0000011111100000', '0000000000000000'];

  /**
   * st: {state:'print'|'idle'|'wait', night, hot:{cur,tgt}, bed:{cur,tgt}, fan, pct, left, total, layer, layers,
   *      file, startsIn, waitPct, fonts:{big}}
   */
  function drawStatus(A, st) {
    const s = screen(), F = A.font_menu, T = A.font_tiny, B = A[st.fonts && st.fonts.big || 'font_9x18Br'];
    const M = A.font_6x10r;
    const deg = (x, y) => { bitmap(s, x, y, ['010', '101', '010']); return x + 4; };

    // Top band (taller): current nozzle, bed, fan; moon at the right in night mode
    // Each column: icon + current value (6x10), target underneath (5x7) only while away from it
    const I = stockIcons(A);
    const col = (x, icon, iy, tx, val, tgt, unit) => {
      bitmap(s, x, iy, icon);
      let e = text(s, M, tx, 10, String(val));
      if (unit === '%') text(s, M, e, 10, '%'); else deg(e + 1, 2);
      // Target only while heating/cooling towards it (more than 5 degrees away)
      if (tgt > 0 && Math.abs(val - tgt) > 5) text(s, F, tx, 21, `${tgt}${DEG}`);
    };
    col(0, st.hot.tgt > 0 ? I.nozzleOn : I.nozzle, 0, 12, st.hot.cur, st.hot.tgt);
    col(42, st.bed.tgt > 0 ? I.bedOn : I.bed, st.bed.tgt > 0 ? 0 : 5, 62, st.bed.cur, st.bed.tgt);
    col(89, I.fan[st.fan > 0 ? (st.frame || 0) % 2 : 0], 0, 102, st.fan, undefined, '%');
    // Row under the band: targets while any heater is away from its target, otherwise labels for the times below
    const away = (h) => h.tgt > 0 && Math.abs(h.cur - h.tgt) > 5;
    if (!(away(st.hot) || away(st.bed))) {
      const labels = st.state === 'print' ? ['TIME LEFT', 'ELAPSED'] : st.state === 'wait' ? ['STARTS IN', 'PRINT TAKES'] : [];
      if (labels.length) { text(s, T, 0, 21, labels[0]); right(s, T, W, 21, labels[1]); }
      hline(s, 0, 13, W, true);
    } else {
      hline(s, 0, 23, W, true);
    }

    // Middle: two values in the same big font, left and right, no labels
    const BASE = 38;
    if (st.state === 'print') {
      text(s, B, 0, BASE, st.left);
      right(s, B, W, BASE, st.elapsed);
      if (st.night) bitmap(s, 56, 24, MOON16);
    } else if (st.state === 'wait') {
      text(s, B, 0, BASE, st.startsIn);
      right(s, B, W, BASE, st.total);
      if (st.night) bitmap(s, 56, 24, MOON16);
    } else if (st.msg) {
      text(s, F, 0, 34, st.msg);  // idle: messages take the place of Ready
    } else {
      text(s, B, 0, BASE, 'Ready');
    }

    // Progress bar with the percentage inside it
    if (st.state !== 'idle') {
      const pct = st.state === 'wait' ? st.waitPct : st.pct;
      frame(s, 0, 42, W, 10);
      if (!(st.paused && st.blink)) box(s, 1, 43, Math.round((W - 2) * pct / 100), 8);  // paused: fill blinks
      const label = st.state === 'wait' ? 'WAITING' : st.paused ? 'PAUSED' : `${pct.toFixed(1)}%`;
      text(s, F, Math.round((W - width(F, label)) / 2), 50, label, 'x');
    }

    // File name, small font, under a rule
    hline(s, 0, 55, W, true);
    if (st.msg && st.state !== 'idle') {
      box(s, 0, 56, W, 8);  // printing: message replaces the file name, inverted
      text(s, T, 1, 63, st.msg, 0);
    } else {
      text(s, T, 0, 63, st.state === 'idle' ? 'Insert card or choose a file' : st.file);
    }
    return s;
  }

  function paint(ctx, s, k, on = '#e8efff', off = '#1a3fd0') {
    ctx.fillStyle = off; ctx.fillRect(0, 0, W * k, H * k);
    ctx.fillStyle = on;
    const g = k >= 5 ? 1 : 0;
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) if (s.px[y * W + x]) ctx.fillRect(x * k, y * k, k - g, k - g);
  }

  const api = { W, H, screen, text, width, bitmap, box, frame, drawStatus, paint, stockIcons, MOON16 };
  if (typeof module !== 'undefined') module.exports = api; else root.LCD = api;
})(this);
