#include "TagLibrary.h"

#include <algorithm>

namespace {
constexpr uint32_t RECORD_MAGIC = 0x54454341;  // "ACET" little-endian.
constexpr uint16_t RECORD_VERSION = 1;
constexpr char TAG_DIRECTORY[] = "/tags";
constexpr char TEMP_FILE[] = "/tags/.save.tmp";

struct __attribute__((packed)) SavedAceRecord {
  uint32_t magic;
  uint16_t version;
  uint16_t recordSize;
  uint32_t id;
  uint8_t sourceUid[10];
  uint8_t sourceUidLength;
  uint8_t pages[28][4];
  char name[40];
  char material[17];
  char colorName[32];
  char colorHex[9];
  char sku[21];
  uint16_t nozzleMin;
  uint16_t nozzleMax;
  uint16_t bedMin;
  uint16_t bedMax;
  uint16_t diameterHundredthsMm;
  uint16_t lengthMeters;
  uint32_t savedSequence;
  uint32_t crc32;
};

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFUL;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xEDB88320UL & (0U - (crc & 1U)));
  }
  return ~crc;
}

bool validRecord(const SavedAceRecord &record) {
  if (record.magic != RECORD_MAGIC || record.version != RECORD_VERSION ||
      record.recordSize != sizeof(SavedAceRecord) || record.sourceUidLength > 10 ||
      record.sourceUidLength == 0)
    return false;
  return record.crc32 == crc32(reinterpret_cast<const uint8_t *>(&record),
                               offsetof(SavedAceRecord, crc32));
}

bool readRecord(const char *path, SavedAceRecord &record) {
  File file = LittleFS.open(path, "r");
  if (!file || file.size() != sizeof(record)) { if (file) file.close(); return false; }
  const size_t count = file.read(reinterpret_cast<uint8_t *>(&record), sizeof(record));
  file.close();
  return count == sizeof(record) && validRecord(record);
}

String filenameFor(uint32_t id) {
  char path[24];
  snprintf(path, sizeof(path), "/tags/%06lu.tag", static_cast<unsigned long>(id));
  return String(path);
}

void copyText(char *destination, size_t size, const char *source) {
  if (!size) return;
  strncpy(destination, source ? source : "", size - 1);
  destination[size - 1] = '\0';
}

void recordFromTag(const AceTagData &tag, uint32_t id, SavedAceRecord &record) {
  record = {};
  record.magic = RECORD_MAGIC;
  record.version = RECORD_VERSION;
  record.recordSize = sizeof(record);
  record.id = id;
  memcpy(record.sourceUid, tag.uid, tag.uidLength);
  record.sourceUidLength = tag.uidLength;
  for (uint8_t page = 4; page <= 31; ++page)
    memcpy(record.pages[page - 4], tag.pages[page], 4);
  copyText(record.name, sizeof(record.name), tag.name);
  copyText(record.material, sizeof(record.material), tag.material);
  copyText(record.colorName, sizeof(record.colorName), tag.colorName);
  copyText(record.colorHex, sizeof(record.colorHex), tag.colorHex);
  copyText(record.sku, sizeof(record.sku), tag.sku);
  record.nozzleMin = tag.nozzleMin; record.nozzleMax = tag.nozzleMax;
  record.bedMin = tag.bedMin; record.bedMax = tag.bedMax;
  record.diameterHundredthsMm = tag.diameterHundredthsMm;
  record.lengthMeters = tag.lengthMeters;
  record.savedSequence = id;
  record.crc32 = crc32(reinterpret_cast<const uint8_t *>(&record),
                       offsetof(SavedAceRecord, crc32));
}

void tagFromRecord(const SavedAceRecord &record, AceTagData &tag) {
  tag = {};
  memcpy(tag.uid, record.sourceUid, record.sourceUidLength);
  tag.uidLength = record.sourceUidLength;
  for (uint8_t page = 4; page <= 31; ++page)
    memcpy(tag.pages[page], record.pages[page - 4], 4);
  copyText(tag.name, sizeof(tag.name), record.name);
  copyText(tag.material, sizeof(tag.material), record.material);
  copyText(tag.colorName, sizeof(tag.colorName), record.colorName);
  copyText(tag.colorHex, sizeof(tag.colorHex), record.colorHex);
  copyText(tag.sku, sizeof(tag.sku), record.sku);
  tag.nozzleMin = record.nozzleMin; tag.nozzleMax = record.nozzleMax;
  tag.bedMin = record.bedMin; tag.bedMax = record.bedMax;
  tag.diameterHundredthsMm = record.diameterHundredthsMm;
  tag.lengthMeters = record.lengthMeters;
  if (record.colorHex[0]) {
    unsigned long rgba = strtoul(record.colorHex, nullptr, 16);
    tag.red = (rgba >> 24) & 0xFF; tag.green = (rgba >> 16) & 0xFF;
    tag.blue = (rgba >> 8) & 0xFF;
  }
  tag.readOk = true;
  tag.aceValid = true;
  String uid;
  for (uint8_t i = 0; i < tag.uidLength; ++i) {
    if (i) uid += ':';
    if (tag.uid[i] < 0x10) uid += '0';
    uid += String(tag.uid[i], HEX);
  }
  uid.toUpperCase(); tag.uidText = uid;
}
}  // namespace

bool TagLibrary::begin() {
  mounted_ = LittleFS.begin(false);
  if (!mounted_) {
    Serial.println("[fs] LittleFS mount FAILED; library disabled (filesystem not formatted)");
    return false;
  }
  if (!LittleFS.exists(TAG_DIRECTORY)) LittleFS.mkdir(TAG_DIRECTORY);
  if (LittleFS.exists(TEMP_FILE)) {
    LittleFS.remove(TEMP_FILE);
    Serial.println("[fs] Removed abandoned /tags/.save.tmp");
  }
  Serial.println("[fs] LittleFS mounted");
  Serial.printf("[fs] Total=%u Used=%u Free=%u\n", totalBytes(), usedBytes(), freeBytes());
  return true;
}

bool TagLibrary::formatAndBegin() {
  Serial.println("[fs] User requested LittleFS format");
  LittleFS.end();
  if (!LittleFS.format()) {
    Serial.println("[fs] LittleFS format FAILED");
    mounted_ = false;
    return false;
  }
  mounted_ = LittleFS.begin(false);
  if (mounted_ && !LittleFS.exists(TAG_DIRECTORY)) LittleFS.mkdir(TAG_DIRECTORY);
  Serial.printf("[fs] Format %s\n", mounted_ ? "PASS" : "FAILED TO MOUNT");
  if (mounted_)
    Serial.printf("[fs] Total=%u Used=%u Free=%u\n", totalBytes(), usedBytes(), freeBytes());
  return mounted_;
}

bool TagLibrary::refresh() {
  entries_.clear(); invalidCount_ = 0;
  if (!mounted_) return false;
  Serial.println("[library] Enumerating records");
  File directory = LittleFS.open(TAG_DIRECTORY);
  if (!directory || !directory.isDirectory()) return false;
  File file = directory.openNextFile();
  while (file) {
    const String path = file.path();
    file.close();
    if (path.endsWith(".tag")) {
      SavedAceRecord record{};
      if (readRecord(path.c_str(), record)) {
        LibraryEntry entry{};
        entry.id = record.id;
        copyText(entry.filename, sizeof(entry.filename), path.c_str());
        copyText(entry.material, sizeof(entry.material), record.material);
        copyText(entry.colorName, sizeof(entry.colorName), record.colorName);
        copyText(entry.sku, sizeof(entry.sku), record.sku);
        unsigned long rgba = strtoul(record.colorHex, nullptr, 16);
        entry.red = (rgba >> 24) & 0xFF; entry.green = (rgba >> 16) & 0xFF;
        entry.blue = (rgba >> 8) & 0xFF;
        entries_.push_back(entry);
      } else {
        ++invalidCount_;
        Serial.printf("[library] Skipped invalid file %s\n", path.c_str());
      }
    }
    file = directory.openNextFile();
  }
  directory.close();
  std::sort(entries_.begin(), entries_.end(),
            [](const LibraryEntry &a, const LibraryEntry &b) { return a.id < b.id; });
  Serial.printf("[library] Loaded %u entries; invalid=%u\n", entries_.size(), invalidCount_);
  return true;
}

bool TagLibrary::exactDuplicate(const AceTagData &tag, uint32_t *existingId) {
  if (!mounted_) return false;
  if (entries_.empty()) refresh();
  for (const auto &entry : entries_) {
    SavedAceRecord record{};
    if (!readRecord(entry.filename, record)) continue;
    const bool sameUid = record.sourceUidLength == tag.uidLength &&
                         !memcmp(record.sourceUid, tag.uid, tag.uidLength);
    if (sameUid) { if (existingId) *existingId = entry.id; return true; }
  }
  return false;
}

LibrarySaveResult TagLibrary::save(const AceTagData &tag, bool allowDuplicate, uint32_t &savedId) {
  savedId = 0;
  if (!mounted_) return LibrarySaveResult::Unavailable;
  if (!tag.readOk || !tag.aceValid) return LibrarySaveResult::InvalidSource;
  if (!allowDuplicate && exactDuplicate(tag)) return LibrarySaveResult::Duplicate;
  if (freeBytes() < sizeof(SavedAceRecord) + 4096) return LibrarySaveResult::Full;
  if (entries_.empty()) refresh();
  uint32_t id = 1;
  for (const auto &entry : entries_) id = max(id, entry.id + 1);
  while (LittleFS.exists(filenameFor(id))) ++id;
  SavedAceRecord record{}; recordFromTag(tag, id, record);
  Serial.printf("[library] Saving ID=%lu\n", static_cast<unsigned long>(id));
  LittleFS.remove(TEMP_FILE);
  File temporary = LittleFS.open(TEMP_FILE, "w");
  if (!temporary) return LibrarySaveResult::WriteFailed;
  const size_t written = temporary.write(reinterpret_cast<const uint8_t *>(&record), sizeof(record));
  temporary.flush(); temporary.close();
  if (written != sizeof(record)) { LittleFS.remove(TEMP_FILE); return LibrarySaveResult::WriteFailed; }
  SavedAceRecord check{};
  if (!readRecord(TEMP_FILE, check)) { LittleFS.remove(TEMP_FILE); return LibrarySaveResult::WriteFailed; }
  const String finalPath = filenameFor(id);
  if (!LittleFS.rename(TEMP_FILE, finalPath)) { LittleFS.remove(TEMP_FILE); return LibrarySaveResult::CommitFailed; }
  savedId = id;
  refresh();
  Serial.printf("[library] Saved %s (%u bytes)\n", finalPath.c_str(), sizeof(record));
  return LibrarySaveResult::Ok;
}

bool TagLibrary::load(uint32_t id, AceTagData &tag) {
  if (!mounted_) return false;
  const String path = filenameFor(id);
  SavedAceRecord record{};
  Serial.printf("[library] Loading ID=%lu\n", static_cast<unsigned long>(id));
  if (!readRecord(path.c_str(), record)) return false;
  tagFromRecord(record, tag);
  Serial.printf("[library] Loaded %s %s\n", tag.material, tag.colorName);
  return true;
}

bool TagLibrary::remove(uint32_t id) {
  if (!mounted_) return false;
  const String path = filenameFor(id);
  Serial.printf("[library] Delete ID=%lu\n", static_cast<unsigned long>(id));
  if (!LittleFS.remove(path)) { Serial.println("[library] Delete FAILED"); return false; }
  refresh();
  Serial.printf("[fs] Deleted %s\n", path.c_str());
  return true;
}
