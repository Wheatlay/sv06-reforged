// Splice mockup/status-section.html (with lcd.js and lcd_assets.json inlined) into sv06-menu.html
// between the status:begin / status:end markers. Usage: node tools/build_mockup.js
const fs = require('fs');
const path = require('path');
const dir = path.join(__dirname, '..', 'mockup');
const read = (f) => fs.readFileSync(path.join(dir, f), 'utf8');

const section = read('status-section.html')
  .replace('/*ASSETS*/', () => read('lcd_assets.json'))
  .replace('/*LCD_JS*/', () => read('lcd.js'));
const page = read('sv06-menu.html');
const re = /<!-- status:begin -->[\s\S]*<!-- status:end -->/;
if (!re.test(page)) throw new Error('status markers not found in sv06-menu.html');
fs.writeFileSync(path.join(dir, 'sv06-menu.html'), page.replace(re, () => `<!-- status:begin -->\n${section}<!-- status:end -->`));
