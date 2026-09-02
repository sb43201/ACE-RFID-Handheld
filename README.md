# ACE RFID Handheld

Portable ESP32 reader, writer, cloner, read-only emulator, and library for Anycubic ACE filament RFID tags.

The handheld combines a Hosyond/LCDWiki E32R35T 3.5-inch touchscreen with a PN532 NFC module. It reads ACE filament metadata, presents an accessible full-size color plate and color name, writes verified presets to compatible rewritable tags, clones ACE payloads, emulates tags to an ACE reader, and stores scanned or generated presets in onboard flash.

> This is an independent community project. It is not affiliated with or endorsed by Anycubic.

## Highlights

- Reads ACE data from NTAG-compatible tags and decodes material, color, SKU, nozzle range, bed range, diameter, and filament length.
- Large color plate and written color name for color-blind accessibility.
- Writes community, catalog, and factory-captured filament presets.
- Clones the complete ACE payload on pages 4–31 and verifies every byte.
- Emulates scanned, saved, and built-in preset tags in read-only PN532 target mode.
- Shows ACE read progress, blocks write commands during emulation, and provides an on-screen Cancel action.
- Detects factory-locked or incompatible tags and avoids reporting false success.
- Saves scanned tags and generated presets to a CRC-protected LittleFS library with duplicate handling, raw view, rewrite, emulation, and delete.
- Touchscreen setup for calibration, beep volume, and screen timeout.
- Battery icon, percentage, and voltage in the header.
- Wi-Fi and Bluetooth stacks are not linked or initialized, keeping both radios off for an offline, lower-power handheld.
- Musical startup, success, and error cues through the onboard speaker.
- Screen dim/off and touch wake based on the SwitchBot Blind Remote power-management pattern.

## Hardware

- Hosyond/LCDWiki E32R35T with ESP32, ST7796 320×480 TFT, and XPT2046 touch
- Elechouse-style PN532 NFC/RFID V3 module configured for SPI
- Common 3.3 V supply and ground

### Bill of materials

| Qty. | Component | Purchase link | Notes |
|---:|---|---|---|
| 1 | Hosyond 3.5-inch 320×480 Touch Screen ESP32 Display (Cheap Yellow Display) | [Amazon](https://a.co/d/01DXrowN) | Main ESP32, touchscreen, display, battery input, and microSD hardware |
| 1 | PN532 NXP NFC/RFID module | [Amazon](https://a.co/d/03boXSBR) | Configure the module for SPI mode |
| 1 | 3.7 V, 1000 mAh LiPo battery | [Amazon](https://a.co/d/0ji3c6oG) | Verify connector type and polarity before connecting |
| 1 | 4 Ω, 3 W mini speaker | [Amazon](https://a.co/d/07AWv2H9) | Audio feedback and startup/success/error cues |
| 1 | 8×8 mm DPDT tactile switch | [Amazon](https://a.co/d/04x0NMgU) | Power switch |
| 1 kit | 1.25 mm JST/Molex connector kit | [Amazon](https://a.co/d/0b7Jxc98) | Internal removable wiring connections |
| As required | NTAG213 RFID tags | [Amazon](https://a.co/d/0dDuntYi) | Rewritable test/destination tags; verify usable pages and lock state before writing |
| As required | Hookup wire, heat-shrink tubing, and small fasteners | — | Final lengths and fasteners depend on the enclosure build |
| 1 set | Printed handheld enclosure | [Enclosure files](enclosure/) | Print the matching front and rear enclosure parts |

Amazon links are provided for convenience and may change or become unavailable. Verify dimensions, voltage, pinout, and connector polarity before ordering or assembly. LiPo connector polarity is not standardized; reversed polarity can damage the handheld or battery.

### PN532 wiring

Disconnect power before changing the PN532 interface switches. Configure the module for **SPI mode**.

| PN532 | E32R35T | Purpose |
|---|---:|---|
| SCK | GPIO18 | VSPI clock |
| MISO | GPIO19 | VSPI controller input |
| MOSI | GPIO23 | VSPI controller output |
| SS | GPIO21 | PN532 chip select |
| VCC | 3.3 V | Power |
| GND | GND | Common ground |
| IRQ | Not connected | Polling is used |
| RSTPD_N | GPIO25 / SCL | Hard Power Down during ESP32 deep sleep |
| RSTO | Not connected | Not required by the current driver |

The PN532 antenna is mounted at the back of the handheld. Present tags to the rear antenna area.

### Complete pin map

| Subsystem | Signal | GPIO |
|---|---|---:|
| TFT | SCLK / MOSI / MISO | 14 / 13 / 12 |
| TFT | CS / DC / backlight | 15 / 2 / 27 |
| Touch | CS / IRQ | 33 / 36 |
| PN532 | SCK / MISO / MOSI / SS | 18 / 19 / 23 / 21 |
| PN532 | RSTPD_N | 25 (E32R35T SCL header) |
| Speaker | audio / active-low enable | 26 / 4 |
| Battery | ADC | 34 |

## Software

- PlatformIO
- Espressif Arduino framework for ESP32
- TFT_eSPI 2.5.43
- XPT2046_Touchscreen
- Adafruit PN532 1.3.4
- LittleFS and Preferences from the ESP32 framework

## Build and upload

1. Install [Visual Studio Code](https://code.visualstudio.com/) and the [PlatformIO extension](https://platformio.org/install/ide?install=vscode).
2. Clone this repository and open its folder in PlatformIO.
3. Connect the E32R35T by USB.
4. Build and upload:

```powershell
platformio run
platformio run --target upload
platformio device monitor
```

The serial monitor uses 115200 baud. Upload speed is configured as 921600 baud; reduce `upload_speed` in `platformio.ini` if a USB adapter is unreliable.

## Using the handheld

- Scan: hold an ACE tag near the antenna on the back.
- Write preset: choose `WRITE PRESET`, select a material and color, review the details, and explicitly arm writing.
- Clone: scan a valid ACE source, select `CLONE`, remove the source, and present a different rewritable destination.
- Emulate a scanned tag: scan it, select `EMU`, remove the source tag, and hold the handheld's PN532 antenna near the ACE sensor.
- Emulate a preset: choose `WRITE PRESET`, select a color, then use the yellow `EMU` button.
- Library: scan a valid ACE tag and select `SAVE`, or use the green `SAVE` button on a preset preview; open `LIBRARY` from the Ready screen to view saved records and emulate them later.
- Setup: long-press the Ready/Scan screen for about 1.2 seconds.

See the [User Manual](docs/USER_MANUAL.md) for complete operating and safety instructions.

## Important tag limitations

- Writing requires a compatible rewritable tag with at least pages 4–31 available.
- The writer currently accepts 7-byte UID destinations for ACE-compatible rewritable media.
- Genuine Anycubic factory tags are commonly locked. They can be read and saved, but attempts to alter them may correctly report `WRITE PROTECTED`.
- Writing and cloning affect only pages 4–31. Manufacturer/UID, lock, capability, and configuration pages are not intentionally changed.
- Never remove or exchange a tag during writing or verification.
- Test with expendable tags before relying on a newly purchased tag type.

## Emulation behavior

Emulation is intentionally read-only. The handheld responds to the ACE reader's page-read commands but refuses NFC write commands. The `READ ONLY - WRITES BLOCKED` message describes this behavior; it does not mean that the saved Library record is locked.

The ACE reader normally requests nine four-page blocks from page `04` through page `24`. The EMU progress display advances after each successful response. `READ COMPLETE` indicates that the complete expected read sequence was served. Use `CANCEL` to leave target mode and return to Library.

Before preset emulation, the firmware performs short passive-reader probes to place the PN532 in a reliable state. This replaces the previously observed need to visit Library between target-mode sessions.

Built-in presets do not originate from a physical NFC tag, so saving one to Library assigns it a deterministic synthetic seven-byte source UID. The same preset always receives the same UID, different preset payloads receive different UIDs, and printer recognition continues to come from the ACE payload fields.

## Data integrity

Writes are followed by a complete read-back of pages 4–31 and byte-for-byte verification. Clone data is captured by value before the destination is accepted. The saved-tag library uses versioned binary records, CRC32 validation, and a temporary-file/rename workflow.

The core read, preset-write, protection-detection, clone, Library, read-only emulation, sound, display, touch, battery, and power-management paths have been exercised on hardware. Saved genuine tags and consecutive built-in presets—including PLA, Silk, ABS, PC, and ASA examples—were recognized by an ACE reader during hardware testing. Users should still validate behavior on their own printer and keep copies of irreplaceable records.

## Technical references

- [ACE tag memory format](docs/ACE_TAG_FORMAT.md)
- [Factory tag captures](docs/FACTORY_TAG_CAPTURES.md)
- [Anycubic catalog preset provenance](docs/ANYCUBIC_CATALOG_PRESETS.md)

## Repository layout

```text
src/audio/     Musical cues and saved volume setting
src/config/    Hardware pin assignment
src/power/     Backlight, sleep, wake, and battery monitoring
src/rfid/      PN532 access, ACE codec, tag model, and presets
src/storage/   LittleFS saved-tag library
src/ui/        Touch handling and all 320×480 screens
docs/          User and technical documentation
images/        Project photographs, screenshots, and wiring images
enclosure/     Printable enclosure STL files and model exports
```

## Disclaimer

RFID writing can permanently alter compatible tags. Use this firmware at your own risk. Keep backups of known-good payloads and do not experiment on the only tag for a filament roll.

## License

This project is released under the [MIT License](LICENSE).
