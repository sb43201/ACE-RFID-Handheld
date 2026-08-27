#pragma once

#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <SPI.h>
#include "AceTag.h"

struct Pn532Status {
  bool found = false;
  bool samConfigured = false;
  uint8_t chip = 0;
  uint8_t firmwareMajor = 0;
  uint8_t firmwareMinor = 0;
  uint8_t support = 0;
};

enum class WritePageResult : uint8_t {
  Ok, Removed, DifferentTag, WriteProtected, Failed, VerifyMismatch
};
enum class ReadPagesFailure : uint8_t { None, PageReadFailed, DifferentTag };

class Pn532Reader {
 public:
  Pn532Reader(uint8_t ss, SPIClass &spi);
  Pn532Status begin();
  bool poll(AceTagData &tag);
  bool readPages(AceTagData &tag);
  bool consumeFieldCleared();
  bool detectTag(AceTagData &tag, uint16_t timeoutMs = 60);
  WritePageResult writePageSafe(uint8_t page, const uint8_t data[4], const AceTagData &destination);
  const Pn532Status &status() const { return status_; }
  ReadPagesFailure lastReadFailure() const { return lastReadFailure_; }

 private:
  static String formatUid(const uint8_t *uid, uint8_t length);
  bool sameUid(const uint8_t *uid, uint8_t length, const AceTagData &tag) const;

  Adafruit_PN532 nfc_;
  Pn532Status status_;
  String activeUid_;
  uint32_t lastPresentMs_ = 0;
  bool armed_ = true;
  bool fieldCleared_ = false;
  ReadPagesFailure lastReadFailure_ = ReadPagesFailure::None;
  static constexpr uint32_t REMOVAL_REARM_MS = 750;
};
