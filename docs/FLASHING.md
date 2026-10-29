# Flashing the SV06

Flashing from the SD card only replaces the firmware. The bootloader that reads the card is not touched, so in
almost every case you can go back by flashing the stock file the same way.

## What you need

- A microSD card, **16 GB or smaller**, formatted **FAT32 with 4096-byte clusters** (Sovol's official instructions).
  A good brand-name card is safer than the one that came with the printer.
- The firmware file:
  - SV06 Reforged: the `.bin` from this repo's Releases
  - stock Sovol V2.0.0_A, for going back: `SV06V2.0.0A_2.24-BCI.bin` (built 24.02.2023) from the official wiki,
    https://wiki.sovol3d.com/en/SV06

## Steps

1. Turn the printer **off**.
2. Copy the `.bin` file to the **root** of the card (not into a folder). Remove any other `.bin` files from the card.
3. **Give the file a name it has never had on this printer**, for example `REFORGE1.BIN`, then `REFORGE2.BIN` for
   the next flash. The bootloader skips a file whose name it has already flashed.
4. **Eject** the card in your operating system before pulling it out.
5. Push the card fully into the printer until it clicks, and turn the printer on.
6. Wait. The screen stays blank or blue for about 30 seconds while it flashes. Don't turn the printer off.
7. SV06 Reforged starts by itself: boot logo, then `2.1.2.9-R1.1.1`. If the saved settings don't match (always after
   stock firmware), it shows an **EEPROM alert**: its "Reset" loads the firmware defaults (about 10 s, don't
   power off). Then set your Z offset again and run Level Bed.
   (Sovol's firmware instead asks to **Reset**. An "Err" after that Reset is normal, says Sovol: select Reset
   again until the main screen appears.)

## If something goes wrong

| Problem | What to do |
|---|---|
| Old firmware still shows | The name was used before or the file wasn't in the root. Rename and try again |
| Blue screen at power-on; over USB (115200 baud) the bootloader repeats "Update failed, No SD/TF-Card or error" | The card isn't read. Power off, push the card fully into the slot until it clicks, power on. Eject the card properly before pulling it from a computer |
| Still on the flashing screen after about a minute | Power cycle once. If it still doesn't start, copy the file again under a new name |
| No Reset prompt / version unchanged | Different card, new file name (Sovol's advice) |
| Still blank with the stock file | Try another card (FAT32, 4096-byte clusters, ≤ 16 GB). Then check that the display and board cables are seated (a loose cable caused exactly this for one community member) |
| "Media init fail" | Reformat the card or use another one |

Last resort: the board can be flashed with an ST-Link programmer.

## Rollback

Same steps with Sovol's stock file. Afterwards run `M502` then `M500` (or Configuration > Restore Defaults, then
Store Settings) so the stock firmware doesn't read SV06 Reforged's saved settings.

After a successful flash, delete the `.bin` from the card so it can't be flashed again by accident.
