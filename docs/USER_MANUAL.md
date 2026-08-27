# ACE RFID Handheld User Manual

## 1. Overview

ACE RFID Handheld reads and manages RFID tags used with Anycubic ACE filament systems. Its main functions are:

- Scan and identify an ACE filament tag
- Display the filament type and a large written color name
- Inspect raw tag pages
- Write a selected filament preset to a compatible blank or rewritable tag
- Clone an existing ACE tag to another compatible tag
- Save scanned ACE tags in the handheld library

The RFID antenna is on the **back** of the handheld.

## 2. Starting the handheld

1. Power on or reset the device.
2. Wait for the Ready screen.
3. Confirm that the screen reports `PN532 ONLINE`.
4. Hold the tag flat near the antenna area on the back.

A musical cue confirms startup and a successful tag detection. The header shows battery percentage and voltage.

## 3. Ready screen

The Ready screen provides two actions:

- `LIBRARY` opens saved tags.
- `WRITE PRESET` opens the filament preset catalog.

The RFID graphic is also the scan target/status area. Long-press the Ready screen for approximately 1.2 seconds to open Setup.

## 4. Scanning a tag

1. Place one tag near the rear antenna.
2. Hold it still while the handheld reads pages 4–31.
3. The result screen shows a large color plate and written color name, followed by material, SKU, temperature ranges, diameter, length, and UID.
4. Remove the tag when reading is complete. The result remains available until another tag is scanned or Home is selected.

Possible results:

- `ACE TAG`: a complete, valid ACE payload was decoded.
- `NFC TAG`: the tag is readable but is not recognized as ACE data.
- `UNSUPPORTED`: the tag type or memory layout is unsuitable.
- `READ ERROR`: the required memory could not be read; remove and retry.

## 5. Result-screen actions

- `RAW` displays the retained bytes for pages 4–31.
- `CLONE` captures the displayed ACE payload as a clone source.
- `SAVE` stores the tag in the onboard library.
- `WRITE` opens the preset writer.
- `HOME` returns to the Ready screen.

## 6. Writing a filament preset

### Select a preset

1. Select `WRITE PRESET`.
2. Choose a material tab such as PLA, PLA+, PETG, TPU, or Other.
3. Use `PREV` and `NEXT` to change pages.
4. Tap a numbered color row.
5. Review the large color plate, color name, material, SKU, and encoded values.
6. Select `WRITE TAG` only when the displayed preset is correct.

### Write the tag

1. Remove all tags until the handheld reports that destination detection is armed.
2. Place one compatible rewritable tag near the rear antenna.
3. Keep that same tag completely still through writing and verification.
4. Wait for `WRITE COMPLETE`, with both Write and Verify marked `PASS`.

Do not remove, slide, stack, or exchange tags during this operation.

### Write failures

- `WRITE PROTECTED`: the tag accepted no effective change or is factory locked.
- `SAME TAG`: a clone destination is the captured source tag.
- `WRITE FAILED`: the tag moved, communication failed, or the memory is incompatible.
- `VERIFY FAILED`: written bytes did not match the intended payload.
- `PARTIAL ACE DATA`: some pages may have changed before the failure. Rewrite the complete intended image before using that tag.

Many genuine Anycubic factory filament tags are protected. Reading them is safe; rewriting may not be possible.

## 7. Cloning a tag

1. Scan a valid ACE source tag.
2. Remove it after the result is displayed.
3. Select `CLONE`.
4. Keep the antenna area clear until destination detection is armed.
5. Present a different compatible rewritable destination tag.
6. Hold it still until clone writing and full verification pass.

Cloning copies the exact 112-byte ACE payload from pages 4–31. It does not copy the source UID or manufacturer pages.

## 8. Saved-tag library

### Save

1. Scan a valid ACE tag.
2. Select `SAVE`.
3. If the same source UID already exists, choose whether to save another copy or cancel.

### Browse

1. From Ready, select `LIBRARY`.
2. Use `PREV` and `NEXT` on the right edge.
3. Tap a row to open its details.

Saved details allow you to view raw data, write a copy to a compatible destination, or delete the record. `STORAGE` shows filesystem capacity and record counts.

If storage has never been initialized and is unavailable, use the red Format action only after confirming that no saved data must be preserved. Formatting erases the handheld library but does not affect RFID tags.

## 9. Setup

Long-press the Ready/Scan screen for approximately 1.2 seconds.

### Touch calibration

Select `Touch calibration`, then touch and release each displayed target accurately. Calibration is stored in nonvolatile memory.

### Beep volume

Tap `Beep volume` to cycle through High, Mid, Low, and Mute. The selected setting is stored.

### Screen sleep

Tap `Screen sleep` to cycle through:

- 30 seconds
- 60 seconds
- 120 seconds
- Always On

For a timed mode, the display dims halfway to the selected off time and turns off at the selected time. RFID scanning continues while the display is off. Tag or touch activity wakes the display. After prolonged inactivity, the handheld can enter deep sleep and wake from the touchscreen interrupt. Always On disables automatic dim, off, and deep sleep.

Select `HOME` to return to Ready.

## 10. Battery display

The header shows:

- Battery icon
- Estimated charge percentage
- Measured battery voltage

Percentage is an estimate derived from voltage and is most useful as a trend. Load, charging state, and battery condition can cause short-term variation.

## 11. Care and troubleshooting

### PN532 not found

- Turn power off.
- Confirm the PN532 is set to SPI mode.
- Check GPIO18, GPIO19, GPIO23, GPIO21, 3.3 V, and ground.
- Confirm that IRQ and RSTO are left unconnected for this firmware.

### Tag is not detected

- Place it against the back antenna area, not the screen.
- Remove other nearby tags.
- Hold it flat and still.
- Try rotating or shifting it slightly across the rear antenna.

### Touch is inaccurate

Open Setup with a long press and repeat Touch calibration.

### Screen does not sleep

Check Setup for `ALWAYS ON`. Writing, cloning, verification, held touch, and active melodies temporarily prevent sleep.

### A factory tag will not write

This is expected for many protected tags. Use a known compatible rewritable tag. Do not repeatedly attempt writes to a valued factory tag.

## 12. Safety and data protection

- Use 3.3 V logic and a common ground.
- Disconnect power before rewiring or changing PN532 switches.
- Test tag writing with expendable media.
- Never assume that a successful command means a successful write; wait for the handheld’s full verification result.
- Save or record known-good tag data before experimentation.
- Do not use a partially written tag in an ACE system until it has been completely rewritten and verified.
