#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

#include "../rfid/AceTag.h"

struct LibraryEntry {
  uint32_t id = 0;
  char filename[24]{};
  char material[17]{};
  char colorName[32]{};
  char sku[21]{};
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

enum class LibrarySaveResult : uint8_t {
  Ok, Unavailable, InvalidSource, Duplicate, Full, WriteFailed, CommitFailed
};

class TagLibrary {
 public:
  bool begin();
  bool formatAndBegin();
  bool available() const { return mounted_; }
  bool refresh();
  const std::vector<LibraryEntry> &entries() const { return entries_; }
  LibrarySaveResult save(const AceTagData &tag, bool allowDuplicate, uint32_t &savedId);
  bool load(uint32_t id, AceTagData &tag);
  bool remove(uint32_t id);
  bool exactDuplicate(const AceTagData &tag, uint32_t *existingId = nullptr);
  size_t totalBytes() const { return mounted_ ? LittleFS.totalBytes() : 0; }
  size_t usedBytes() const { return mounted_ ? LittleFS.usedBytes() : 0; }
  size_t freeBytes() const { return totalBytes() - usedBytes(); }
  uint16_t invalidCount() const { return invalidCount_; }

 private:
  bool mounted_ = false;
  uint16_t invalidCount_ = 0;
  std::vector<LibraryEntry> entries_;
};
