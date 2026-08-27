#include "Pn532Reader.h"

Pn532Reader::Pn532Reader(uint8_t ss, SPIClass &spi) : nfc_(ss, &spi) {}

Pn532Status Pn532Reader::begin() {
  status_ = {};
  nfc_.begin();
  const uint32_t version = nfc_.getFirmwareVersion();
  if (!version) return status_;

  status_.found = true;
  status_.chip = (version >> 24) & 0xFF;
  status_.firmwareMajor = (version >> 16) & 0xFF;
  status_.firmwareMinor = (version >> 8) & 0xFF;
  status_.support = version & 0xFF;
  status_.samConfigured = nfc_.SAMConfig();
  if (status_.samConfigured) nfc_.setPassiveActivationRetries(0x01);
  return status_;
}

bool Pn532Reader::poll(AceTagData &tag) {
  uint8_t uid[10]{};
  uint8_t uidLength = 0;
  const bool present = nfc_.readPassiveTargetID(
      PN532_MIFARE_ISO14443A, uid, &uidLength, 80);

  if (!present) {
    if (!armed_ && millis() - lastPresentMs_ >= REMOVAL_REARM_MS) {
      armed_ = true;
      activeUid_ = "";
      fieldCleared_ = true;
      Serial.println("[rfid] Field clear; scanner re-armed");
    }
    return false;
  }

  lastPresentMs_ = millis();
  const String detected = formatUid(uid, uidLength);
  if (!armed_ || detected == activeUid_) return false;

  activeUid_ = detected;
  armed_ = false;
  tag = AceTagData{};
  memcpy(tag.uid, uid, uidLength);
  tag.uidLength = uidLength;
  tag.uidText = detected;
  return true;
}

bool Pn532Reader::consumeFieldCleared() {
  const bool value = fieldCleared_;
  fieldCleared_ = false;
  return value;
}

bool Pn532Reader::detectTag(AceTagData &tag, uint16_t timeoutMs) {
  uint8_t uid[10]{};
  uint8_t length = 0;
  if (!nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length, timeoutMs)) return false;
  tag = AceTagData{};
  memcpy(tag.uid, uid, length);
  tag.uidLength = length;
  tag.uidText = formatUid(uid, length);
  return true;
}

bool Pn532Reader::sameUid(const uint8_t *uid, uint8_t length, const AceTagData &tag) const {
  return length == tag.uidLength && memcmp(uid, tag.uid, length) == 0;
}

WritePageResult Pn532Reader::writePageSafe(uint8_t page, const uint8_t data[4],
                                           const AceTagData &destination) {
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    if (nfc_.ntag2xx_WritePage(page, const_cast<uint8_t *>(data))) {
      // Allow the NTAG EEPROM write cycle to settle. Verification is done as
      // one mandatory full read-back after all 28 writes, avoiding a second
      // redundant read transaction after every page.
      delay(8);
      return WritePageResult::Ok;
    }
    if (attempt == 2) break;
    uint8_t uid[10]{};
    uint8_t length = 0;
    if (!nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length, 120))
      return WritePageResult::Removed;
    if (!sameUid(uid, length, destination)) return WritePageResult::DifferentTag;
    Serial.printf("[write] P%02u retry=%u same UID confirmed\n", page, attempt + 1);
  }
  // If all write commands failed but the page remains readable and exactly
  // unchanged, report a locked/incompatible page rather than implying that
  // it was partially modified.
  uint8_t unchanged[4]{};
  if (nfc_.ntag2xx_ReadPage(page, unchanged) &&
      !memcmp(unchanged, destination.pages[page], 4))
    return WritePageResult::WriteProtected;
  return WritePageResult::Failed;
}

bool Pn532Reader::readPages(AceTagData &tag) {
  lastReadFailure_ = ReadPagesFailure::None;
  Serial.println("[ace] Reading pages 4-31");
  for (uint8_t page = 4; page <= 31; ++page) {
    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3 && !ok; ++attempt) {
      ok = nfc_.ntag2xx_ReadPage(page, tag.pages[page]);
      if (!ok && attempt < 2) {
        Serial.printf("[ace] Page %u: retry %u; reselecting target\n", page, attempt + 1);
        uint8_t retryUid[10]{};
        uint8_t retryUidLength = 0;
        const bool reacquired = nfc_.readPassiveTargetID(
            PN532_MIFARE_ISO14443A, retryUid, &retryUidLength, 120);
        if (reacquired &&
            (retryUidLength != tag.uidLength || memcmp(retryUid, tag.uid, tag.uidLength) != 0)) {
          Serial.println("[ace] ERROR: different tag entered field during retry");
          lastReadFailure_ = ReadPagesFailure::DifferentTag;
          tag.readOk = false;
          return false;
        }
        delay(8);
      }
    }
    if (!ok) {
      Serial.printf("[ace] ERROR reading page %u\n", page);
      lastReadFailure_ = ReadPagesFailure::PageReadFailed;
      tag.readOk = false;
      tag.failedPage = page;
      return false;
    }
  }
  tag.readOk = true;
  Serial.println("[ace] Raw page read complete");
  return true;
}

String Pn532Reader::formatUid(const uint8_t *uid, uint8_t length) {
  String text;
  text.reserve(length * 3);
  for (uint8_t i = 0; i < length; ++i) {
    if (i) text += ':';
    if (uid[i] < 0x10) text += '0';
    text += String(uid[i], HEX);
  }
  text.toUpperCase();
  return text;
}
