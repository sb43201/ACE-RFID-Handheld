#include <Arduino.h>
#include <SPI.h>

#include "config/Hardware.h"
#include "audio/MelodyPlayer.h"
#include "rfid/Pn532Reader.h"
#include "rfid/Pn532TargetEmulator.h"
#include "rfid/AceCodec.h"
#include "rfid/AcePresets.h"
#include "power/PowerManager.h"
#include "power/BatteryMonitor.h"
#include "ui/Ui.h"
#include "storage/TagLibrary.h"

Ui ui;
SPIClass pn532Spi(VSPI);
Pn532Reader reader(Hardware::PN532_SS, pn532Spi);
Pn532TargetEmulator targetEmulator(Hardware::PN532_SS, pn532Spi);
MelodyPlayer melody;
PowerManager power;
BatteryMonitor battery;
TagLibrary library;

enum class WriteState : uint8_t {
  Idle, WaitingForClear, WaitingForDestination, Writing, Verifying, Success, Failed
};
enum class OperationMode : uint8_t { None, PresetWrite, Clone, SavedCopy };
WriteState writeState = WriteState::Idle;
OperationMode operationMode = OperationMode::None;
AceTagData intendedTag;
AceTagData destinationTag;
AceTagData cloneSource;
uint8_t writePage = 4;
uint32_t clearSinceMs = 0;
bool wroteAnyPage = false;
uint32_t writeStartedMs = 0;
uint8_t verificationReadAttempt = 0;

void updateEmulationProgress(uint8_t startPage) {
  ui.showEmulationProgress(startPage);
}

bool checkEmulationCancel() {
  return ui.emulationCancelRequested();
}

void assignPresetUid(AceTagData &tag) {
  // Saved records require a UID. Built-in presets have no physical source
  // tag, so derive a stable seven-byte identifier from their ACE payload.
  uint64_t hash = 1469598103934665603ULL;
  for (uint8_t page = 4; page <= 31; ++page) {
    for (uint8_t column = 0; column < 4; ++column) {
      hash ^= tag.pages[page][column];
      hash *= 1099511628211ULL;
    }
  }
  tag.uidLength = 7;
  tag.uid[0] = 0x04;
  for (uint8_t i = 1; i < 7; ++i) tag.uid[i] = hash >> ((i - 1) * 8);
  String uid;
  for (uint8_t i = 0; i < tag.uidLength; ++i) {
    if (i) uid += ':';
    if (tag.uid[i] < 0x10) uid += '0';
    uid += String(tag.uid[i], HEX);
  }
  uid.toUpperCase();
  tag.uidText = uid;
}

void emulateTag(const AceTagData &tag, bool fromSaved, bool fromPreset = false) {
  if (!tag.readOk || !tag.aceValid) {
    ui.showEmulationResult(false, "Valid ACE tag data required");
    melody.play(MelodyPlayer::Cue::Error);
    return;
  }
  Serial.printf("[emu] Source=%s UID=%s SKU=%s\n",
                fromSaved ? "LIBRARY" : fromPreset ? "PRESET" : "PRESENT",
                tag.uidText.c_str(), tag.sku);
  ui.showEmulationWaiting(tag, fromSaved, fromPreset);
  power.wakeDisplayForRfid();

  if (fromPreset) {
    // Library/Saved screens leave normal reader polling active, but the
    // Write/Preset workflow intentionally suppresses reader.poll(). After a
    // completed target session the PN532 needs an initiator command cycle
    // before it will reliably ACK TgInitAsTarget again. Mirror the Library
    // path here without allowing the discarded probe to change the UI.
    Serial.println("[emu] Priming PN532 reader state before preset target mode");
    AceTagData ignoredProbe;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
      reader.detectTag(ignoredProbe, 80);
      delay(10);
    }
  }

  const EmulationResult result = targetEmulator.run(
      tag, 60000, updateEmulationProgress, checkEmulationCancel);

  // Target mode changes the PN532 state. A hardware reset guarantees that
  // normal handheld reader mode is restored even after activation timeout.
  digitalWrite(Hardware::PN532_RSTPD_N, LOW);
  delay(2);
  digitalWrite(Hardware::PN532_RSTPD_N, HIGH);
  delay(10);
  const Pn532Status restored = reader.begin();
  Serial.printf("[emu] Reader restore %s\n",
                restored.found && restored.samConfigured ? "PASS" : "FAILED");

  if (result == EmulationResult::Cancelled) {
    Serial.println("[emu] Cancelled by user; returning to library");
    ui.showLibrary(library.entries(), library.available(), library.totalBytes(),
                   library.usedBytes(), library.invalidCount());
    return;
  }

  const bool complete = result == EmulationResult::Complete;
  const char *detail = complete ? "ACE read pages 4-39" :
                       result == EmulationResult::Interrupted ? "ACE scan was interrupted" :
                       result == EmulationResult::Timeout ? "No ACE reader detected" :
                       "PN532 target transport error";
  ui.showEmulationResult(complete, detail);
  melody.play(complete ? MelodyPlayer::Cue::TagFound : MelodyPlayer::Cue::Error);
}

bool isCopyMode() {
  return operationMode == OperationMode::Clone || operationMode == OperationMode::SavedCopy;
}

const char *operationLogName() {
  return operationMode == OperationMode::Clone ? "clone" :
         operationMode == OperationMode::SavedCopy ? "library" : "write";
}

void beginWriteAttempt() {
  operationMode = OperationMode::PresetWrite;
  ui.setCloneMode(false);
  AcePresets::buildTag(ui.selectedPreset(), intendedTag);
  Serial.printf("[write] Selected preset: %s\n", ui.selectedPreset().name);
  Serial.println("[write] Intended ACE image:");
  for (uint8_t page = 4; page <= 31; ++page)
    Serial.printf("[write] P%02u: %02X %02X %02X %02X\n", page,
                  intendedTag.pages[page][0], intendedTag.pages[page][1],
                  intendedTag.pages[page][2], intendedTag.pages[page][3]);
  writeState = WriteState::WaitingForClear;
  clearSinceMs = 0;
  wroteAnyPage = false;
}

void beginClone(bool captureSource) {
  operationMode = OperationMode::Clone;
  ui.setCloneMode(true);
  if (captureSource) {
    cloneSource = ui.retainedTag();  // Dedicated by-value snapshot; never replaced by destination scans.
    if (!cloneSource.readOk || !cloneSource.aceValid) {
      Serial.println("[clone] FAIL SOURCE NOT ACE: retained source is incomplete or invalid");
      ui.showWriteFailure("SOURCE NOT ACE", "Valid ACE source required", false);
      melody.play(MelodyPlayer::Cue::Error);
      writeState = WriteState::Failed;
      return;
    }
    Serial.printf("[clone] Source captured UID=%s material=%s color=%s SKU=%s\n",
                  cloneSource.uidText.c_str(), cloneSource.material,
                  cloneSource.colorName, cloneSource.sku);
    Serial.println("[clone] Source payload pages 4-31:");
    for (uint8_t page = 4; page <= 31; ++page)
      Serial.printf("[clone] P%02u: %02X %02X %02X %02X\n", page,
                    cloneSource.pages[page][0], cloneSource.pages[page][1],
                    cloneSource.pages[page][2], cloneSource.pages[page][3]);
  }
  intendedTag = cloneSource;  // Exact raw ACE user payload, including reserved bytes.
  writeState = WriteState::WaitingForClear;
  clearSinceMs = 0;
  wroteAnyPage = false;
  ui.showCloneCaptured(cloneSource);
  melody.play(MelodyPlayer::Cue::TagFound);
}

void beginSavedCopy(bool captureSource) {
  operationMode = OperationMode::SavedCopy;
  ui.setCloneMode(true);
  if (captureSource) cloneSource = ui.selectedSavedTag();
  intendedTag = cloneSource;
  writeState = WriteState::WaitingForClear;
  clearSinceMs = 0;
  wroteAnyPage = false;
  ui.showSavedCopyCaptured(cloneSource);
  Serial.printf("[library] Write-copy armed from ID=%lu; pages 4-31 retained\n",
                static_cast<unsigned long>(ui.selectedLibraryId()));
}

void failWrite(const char *title, const String &detail) {
  Serial.printf("[%s] FAIL %s: %s partial=%s\n",
                operationLogName(),
                title, detail.c_str(), wroteAnyPage ? "YES" : "NO");
  ui.showWriteFailure(title, detail.c_str(), wroteAnyPage);
  melody.play(MelodyPlayer::Cue::Error);
  writeState = WriteState::Failed;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[boot] ACE RFID Handheld");

  // This handheld is fully offline. No Wi-Fi, Bluetooth Classic, or BLE
  // library is linked or initialized, so both ESP32 radios remain off.
  Serial.println("[power] WiFi=OFF Bluetooth=OFF (not initialized)");

  ui.begin();
  battery.begin();
  ui.updateBattery(battery.percent(), battery.millivolts());
  // TFT_eSPI configures GPIO27 during tft.init(). Attach backlight PWM only
  // afterwards so TFT initialization cannot replace the LEDC output.
  power.begin();
  melody.begin();
  ui.calibrateIfNeeded();
  ui.showBoot();

  library.begin();  // Failure only disables SAVE/LIBRARY; RFID remains operational.

  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);  // Keep onboard microSD deselected on shared VSPI.
  pn532Spi.begin(Hardware::PN532_SCK, Hardware::PN532_MISO,
                 Hardware::PN532_MOSI, Hardware::PN532_SS);
  Serial.printf("[spi] PN532 VSPI started SCK=%u MISO=%u MOSI=%u SS=%u\n",
                Hardware::PN532_SCK, Hardware::PN532_MISO,
                Hardware::PN532_MOSI, Hardware::PN532_SS);

  const Pn532Status status = reader.begin();
  if (!status.found) {
    Serial.println("[rfid] PN532 not detected");
    melody.play(MelodyPlayer::Cue::Error);
  } else {
    Serial.printf("[rfid] PN532 found chip=0x%02X firmware=%u.%u support=0x%02X\n",
                  status.chip, status.firmwareMajor, status.firmwareMinor,
                  status.support);
    Serial.printf("[rfid] SAM configuration %s\n",
                  status.samConfigured ? "successful" : "FAILED");
    melody.play(status.samConfigured ? MelodyPlayer::Cue::Startup
                                     : MelodyPlayer::Cue::Error);
  }
  ui.showReaderStatus(status);
}

void loop() {
  melody.update();
  if (battery.update()) ui.updateBattery(battery.percent(), battery.millivolts());
  const UiTouchSample touch = ui.sampleTouch();
  power.handleTouch(touch.down);
  const UiAction action = ui.handleTouch(touch, power.controlsAllowed());
  if (action == UiAction::OpenSetup)
    ui.showSetup(melody.volumeLabel(), power.screenTimeoutSeconds());
  if (action == UiAction::CalibrateTouch) {
    ui.runTouchCalibration();
    ui.showSetup(melody.volumeLabel(), power.screenTimeoutSeconds());
  }
  if (action == UiAction::CycleVolume) {
    const uint8_t next = (static_cast<uint8_t>(melody.volume()) + 1) % 4;
    melody.setVolume(static_cast<MelodyPlayer::Volume>(next));
    if (melody.volume() != MelodyPlayer::Volume::Mute)
      melody.play(MelodyPlayer::Cue::TagFound);
    ui.showSetup(melody.volumeLabel(), power.screenTimeoutSeconds());
  }
  if (action == UiAction::CycleScreenTimeout) {
    const uint16_t current = power.screenTimeoutSeconds();
    const uint16_t next = current == 0 ? 30 : current == 30 ? 60 : current == 60 ? 120 : 0;
    power.setScreenTimeoutSeconds(next);
    ui.showSetup(melody.volumeLabel(), power.screenTimeoutSeconds());
  }
  if (action == UiAction::CloseSetup) ui.showReaderStatus(reader.status());
  if (action == UiAction::OpenLibrary || action == UiAction::RefreshLibrary) {
    if (library.available()) library.refresh();
    ui.showLibrary(library.entries(), library.available(), library.totalBytes(),
                   library.usedBytes(), library.invalidCount());
  }
  if (action == UiAction::FormatStorage) {
    const bool ok = library.formatAndBegin();
    if (ok) library.refresh();
    ui.showLibrary(library.entries(), library.available(), library.totalBytes(),
                   library.usedBytes(), library.invalidCount());
    melody.play(ok ? MelodyPlayer::Cue::TagFound : MelodyPlayer::Cue::Error);
  }
  if (action == UiAction::SaveTag || action == UiAction::SaveCopy) {
    const bool allowDuplicate = action == UiAction::SaveCopy;
    uint32_t savedId = 0;
    const LibrarySaveResult result = library.save(ui.retainedTag(), allowDuplicate, savedId);
    if (result == LibrarySaveResult::Duplicate) {
      ui.showSaveDuplicate();
    } else {
      const bool ok = result == LibrarySaveResult::Ok;
      const char *message = ok ? "Saved to internal flash" :
                            result == LibrarySaveResult::Unavailable ? "Internal storage unavailable" :
                            result == LibrarySaveResult::Full ? "Storage full - delete saved tags" :
                            result == LibrarySaveResult::InvalidSource ? "Valid ACE source required" :
                            result == LibrarySaveResult::CommitFailed ? "Could not commit saved file" :
                            "Could not write saved file";
      ui.showSaveResult(ok, savedId, message);
      melody.play(ok ? MelodyPlayer::Cue::TagFound : MelodyPlayer::Cue::Error);
    }
  }
  if (action == UiAction::OpenSaved) {
    AceTagData saved;
    if (library.load(ui.selectedLibraryId(), saved)) ui.showSavedDetail(ui.selectedLibraryId(), saved);
    else ui.showSaveResult(false, 0, "Saved record is invalid or missing");
  }
  if (action == UiAction::ConfirmDelete) {
    if (library.remove(ui.selectedLibraryId())) {
      ui.showLibrary(library.entries(), true, library.totalBytes(), library.usedBytes(),
                     library.invalidCount());
      melody.play(MelodyPlayer::Cue::TagFound);
    } else {
      ui.showDeleteResult(false);
      melody.play(MelodyPlayer::Cue::Error);
    }
  }
  if (action == UiAction::WriteSaved) beginSavedCopy(true);
  if (action == UiAction::EmulatePresent) {
    emulateTag(ui.retainedTag(), false);
    return;
  }
  if (action == UiAction::EmulateSaved) {
    emulateTag(ui.selectedSavedTag(), true);
    return;
  }
  if (action == UiAction::EmulatePreset) {
    AceTagData presetTag;
    AcePresets::buildTag(ui.selectedPreset(), presetTag);
    AceCodec::validateAndDecode(presetTag);
    emulateTag(presetTag, false, true);
    return;
  }
  if (action == UiAction::SavePreset) {
    AceTagData presetTag;
    AcePresets::buildTag(ui.selectedPreset(), presetTag);
    AceCodec::validateAndDecode(presetTag);
    assignPresetUid(presetTag);
    uint32_t savedId = 0;
    const LibrarySaveResult result = library.save(presetTag, false, savedId);
    const bool ok = result == LibrarySaveResult::Ok;
    const char *message = ok ? "Preset saved to Library" :
                          result == LibrarySaveResult::Duplicate ? "Preset already in Library" :
                          result == LibrarySaveResult::Unavailable ? "Internal storage unavailable" :
                          result == LibrarySaveResult::Full ? "Storage full - delete saved tags" :
                          result == LibrarySaveResult::InvalidSource ? "Preset data is invalid" :
                          result == LibrarySaveResult::CommitFailed ? "Could not commit saved file" :
                          "Could not write saved file";
    ui.showSaveResult(ok, savedId, message, &presetTag, true);
    melody.play(ok ? MelodyPlayer::Cue::TagFound : MelodyPlayer::Cue::Error);
  }
  if (action == UiAction::CloneStart) beginClone(true);
  if (action == UiAction::WriteArmed) beginWriteAttempt();
  if (action == UiAction::WriteRetry) {
    if (operationMode == OperationMode::Clone) beginClone(false);
    else if (operationMode == OperationMode::SavedCopy) beginSavedCopy(false);
    else beginWriteAttempt();
  }
  if (action == UiAction::WriteCancel || action == UiAction::WriteDone) {
    writeState = WriteState::Idle;
    operationMode = OperationMode::None;
    ui.setCloneMode(false);
  }
  const bool physicalOperationActive =
      writeState == WriteState::WaitingForClear ||
      writeState == WriteState::WaitingForDestination ||
      writeState == WriteState::Writing || writeState == WriteState::Verifying;
  power.update(melody.playing() || physicalOperationActive);
  if (!reader.status().found || !reader.status().samConfigured) {
    delay(20);
    return;
  }

  if (writeState == WriteState::WaitingForClear) {
    AceTagData seen;
    if (reader.detectTag(seen, 45)) clearSinceMs = 0;
    else {
      if (!clearSinceMs) clearSinceMs = millis();
      if (millis() - clearSinceMs >= 750) {
        writeState = WriteState::WaitingForDestination;
        if (isCopyMode()) ui.showCloneWaiting(cloneSource);
        else ui.showWriteWaiting(false);
        Serial.printf("[%s] Field clear; destination detection armed\n",
                      operationLogName());
      }
    }
    delay(5); return;
  }

  if (writeState == WriteState::WaitingForDestination) {
    if (!reader.detectTag(destinationTag, 60)) { delay(5); return; }
    power.wakeDisplayForRfid();
    Serial.printf("[%s] Destination UID=%s\n",
                  operationLogName(),
                  destinationTag.uidText.c_str());
    if (operationMode == OperationMode::Clone &&
        destinationTag.uidLength == cloneSource.uidLength &&
        memcmp(destinationTag.uid, cloneSource.uid, cloneSource.uidLength) == 0) {
      failWrite("SAME TAG", "Source cannot be destination"); return;
    }
    if (destinationTag.uidLength != 7) {
      failWrite("UNSUPPORTED TAG", "Expected 7-byte NTAG UID"); return;
    }
    // A complete read proves that the required user-memory range exists.
    if (!reader.readPages(destinationTag)) {
      if (reader.lastReadFailure() == ReadPagesFailure::DifferentTag) {
        failWrite("DIFFERENT TAG", "UID changed during inspection"); return;
      }
      failWrite("UNSUPPORTED TAG", "Pages 4-31 not readable"); return;
    }
    if (isCopyMode())
      melody.play(MelodyPlayer::Cue::Startup);
    writePage = 4;
    wroteAnyPage = false;
    writeStartedMs = millis();
    writeState = WriteState::Writing;
    ui.showWriteProgress(isCopyMode() ? "WRITING COPY" : "WRITING TAG", 0, 28);
    return;
  }

  if (writeState == WriteState::Writing) {
    const WritePageResult result = reader.writePageSafe(writePage, intendedTag.pages[writePage], destinationTag);
    if (result != WritePageResult::Ok) {
      if (result != WritePageResult::WriteProtected) wroteAnyPage = true;
      const char *reason = result == WritePageResult::DifferentTag ? "DIFFERENT TAG" :
                           result == WritePageResult::Removed ? "TAG REMOVED" :
                           result == WritePageResult::WriteProtected ?
                             (isCopyMode() ? "PROTECTED TAG" : "WRITE PROTECTED") :
                           result == WritePageResult::VerifyMismatch ? "VERIFY MISMATCH" : "WRITE FAILED";
      const String detail = result == WritePageResult::WriteProtected &&
                            isCopyMode()
                                ? "Factory tag cannot be overwritten"
                                : "Page " + String(writePage);
      failWrite(reason, detail); return;
    }
    wroteAnyPage = true;
    Serial.printf("[%s] P%02u: OK\n",
                  operationLogName(), writePage);
    ui.showWriteProgress(isCopyMode() ? "WRITING COPY" : "WRITING TAG",
                         writePage - 3, 28);
    if (++writePage > 31) {
      Serial.printf("[%s] All pages written in %lu ms; starting full verification\n",
                    operationLogName(),
                    millis() - writeStartedMs);
      writeState = WriteState::Verifying;
      verificationReadAttempt = 0;
      ui.showWriteProgress("VERIFYING", 0, 28);
    }
    return;
  }

  if (writeState == WriteState::Verifying) {
    ++verificationReadAttempt;
    AceTagData actual;
    if (!reader.detectTag(actual, 120)) {
      if (verificationReadAttempt < 3) {
        Serial.printf("[verify] Tag reselect failed; full-read retry %u of 3\n",
                      verificationReadAttempt + 1);
        return;
      }
      failWrite("TAG REMOVED", "Destination unavailable for verify"); return;
    }
    if (actual.uidLength != destinationTag.uidLength ||
        memcmp(actual.uid, destinationTag.uid, actual.uidLength) != 0) {
      failWrite("DIFFERENT TAG", "UID changed before verification"); return;
    }
    if (!reader.readPages(actual)) {
      if (reader.lastReadFailure() == ReadPagesFailure::DifferentTag) {
        failWrite("DIFFERENT TAG", "UID changed during verification"); return;
      }
      if (verificationReadAttempt < 3) {
        Serial.printf("[verify] Full read failed; retry %u of 3 with same UID required\n",
                      verificationReadAttempt + 1);
        return;
      }
      failWrite("VERIFY READ FAILED", "Could not read tag after 3 attempts"); return;
    }
    int8_t mismatchPage = -1;
    int8_t mismatchByte = -1;
    bool changedFromOriginal = false;
    for (uint8_t page = 4; page <= 31; ++page) {
      for (uint8_t byte = 0; byte < 4; ++byte) {
        if (actual.pages[page][byte] != destinationTag.pages[page][byte])
          changedFromOriginal = true;
        if (mismatchPage < 0 &&
            actual.pages[page][byte] != intendedTag.pages[page][byte]) {
          mismatchPage = page;
          mismatchByte = byte;
        }
      }
      if (mismatchPage < 0) Serial.printf("[verify] P%02u PASS\n", page);
      ui.showWriteProgress("VERIFYING", page - 3, 28);
    }
    if (mismatchPage >= 0) {
      Serial.printf("[%s] VERIFY FAIL page=%d byte=%d expected=0x%02X actual=0x%02X changed=%s\n",
                    isCopyMode() ? operationLogName() : "verify",
                    mismatchPage, mismatchByte,
                    intendedTag.pages[mismatchPage][mismatchByte],
                    actual.pages[mismatchPage][mismatchByte],
                    changedFromOriginal ? "YES" : "NO");
      if (!changedFromOriginal) {
        wroteAnyPage = false;
        if (isCopyMode())
          failWrite("PROTECTED TAG", "Factory tag cannot be overwritten");
        else failWrite("WRITE PROTECTED", "Factory/locked tag unchanged");
      } else {
        failWrite("VERIFY FAILED", "Mismatch page " + String(mismatchPage));
      }
      return;
    }
    Serial.printf("[%s] All cloned/written bytes PASS\n",
                  isCopyMode() ? operationLogName() : "verify");
    Serial.printf("[verify] Write plus verification completed in %lu ms\n",
                  millis() - writeStartedMs);
    if (isCopyMode())
      ui.showCloneSuccess(cloneSource, destinationTag.uidText);
    else ui.showWriteSuccess(destinationTag.uidText);
    melody.play(MelodyPlayer::Cue::TagFound);
    writeState = WriteState::Success;
    clearSinceMs = 0;
    return;
  }

  if (writeState == WriteState::Success) {
    AceTagData seen;
    if (reader.detectTag(seen, 45)) clearSinceMs = 0;
    else {
      if (!clearSinceMs) clearSinceMs = millis();
      if (millis() - clearSinceMs >= 750) {
        writeState = WriteState::Idle;
        // Keep the verified result visible until DONE is explicitly tapped.
        // Returning directly to Scan placed WRITE PRESET under the same footer
        // area and could turn a release/noisy sample into unwanted navigation.
        Serial.printf("[%s] Destination removed; press DONE to return to scanner\n",
                      operationLogName());
      }
    }
    return;
  }

  if (writeState == WriteState::Failed) { delay(5); return; }
  if (ui.writeWorkflowActive()) { delay(5); return; }

  AceTagData tag;
  if (reader.poll(tag)) {
    power.wakeDisplayForRfid();
    Serial.printf("[rfid] Tag detected UID=%s\n", tag.uidText.c_str());
    if (!reader.readPages(tag)) {
      if (tag.uidLength == 4) ui.showUnsupportedTag(tag);
      else ui.showReadError(tag);
      melody.play(MelodyPlayer::Cue::Error);
    } else {
      AceCodec::validateAndDecode(tag);
      AceCodec::dump(tag);
      if (tag.aceValid) ui.showAceTag(tag);
      else ui.showGenericTag(tag);
      melody.play(MelodyPlayer::Cue::TagFound);
    }
  }
  if (reader.consumeFieldCleared()) ui.onFieldCleared();
  delay(5);
}
