#pragma once

#include <Preferences.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "../rfid/Pn532Reader.h"
#include "../rfid/AceTag.h"
#include "../rfid/AcePresets.h"
#include "../storage/TagLibrary.h"

enum class UiScreen : uint8_t {
  Scan, AceResult, GenericTag, UnsupportedTag, ReadError, RawInspector,
  WriteSelect, WritePreview, WriteWaiting, WriteProgress, WriteSuccess, WriteFailure,
  CloneCaptured, CloneWaiting, CloneSuccess, LibraryList, SavedDetail, SavedRaw,
  DeleteConfirm, SaveDuplicate, SaveResult, StorageInfo, FormatConfirm,
  EraseConfirm, Setup
  , EmulateWaiting, EmulateResult
};

enum class UiAction : uint8_t {
  None, WriteArmed, WriteCancel, WriteRetry, WriteDone, CloneStart, SaveTag,
  SaveCopy, OpenLibrary, OpenSaved, WriteSaved, DeleteSaved, ConfirmDelete,
  RefreshLibrary, FormatStorage, OpenSetup, CalibrateTouch, CycleVolume,
  CycleScreenTimeout, CloseSetup
  , EmulatePresent, EmulateSaved, EmulatePreset
};

struct UiTouchSample {
  bool down = false;
  int16_t x = 0;
  int16_t y = 0;
};

// Adafruit GFX and TFT_eSPI share the classic fixed-width 5x7 bitmap face.
// The SwitchBot Blind Remote uses that face at text sizes 1 and 2. This thin
// adapter preserves the existing UI call sites/datums while selecting the
// matching face and mapping the former Font2/Font4 roles to those two sizes.
class BlindRemoteTypefaceTft : public TFT_eSPI {
 public:
  int16_t drawString(const String &text, int32_t x, int32_t y, uint8_t role = 2) {
    setTextFont(1);
    setTextSize(role >= 4 ? 2 : 1);
    return TFT_eSPI::drawString(text, x, y, 1);
  }
  int16_t drawString(const char *text, int32_t x, int32_t y, uint8_t role = 2) {
    return drawString(String(text), x, y, role);
  }
};

class Ui {
 public:
  Ui();
  void begin();
  void calibrateIfNeeded();
  void showBoot();
  void showReaderStatus(const Pn532Status &status);
  void showUid(const String &uid);
  void showAceTag(const AceTagData &tag);
  void showGenericTag(const AceTagData &tag);
  void showUnsupportedTag(const AceTagData &tag);
  void showReadError(const AceTagData &tag);
  UiTouchSample sampleTouch();
  UiAction handleTouch(const UiTouchSample &sample, bool controlsAllowed);
  void onFieldCleared();
  void showWriteSelect();
  void showWriteWaiting(bool fieldMustClear);
  void showWriteProgress(const char *phase, uint8_t completed, uint8_t total);
  void showWriteSuccess(const String &uid);
  void showWriteFailure(const char *title, const char *detail, bool partial);
  void showCloneCaptured(const AceTagData &source);
  void showSavedCopyCaptured(const AceTagData &source);
  void showCloneWaiting(const AceTagData &source);
  void showCloneSuccess(const AceTagData &source, const String &uid);
  void restoreRetainedAce();
  void showLibrary(const std::vector<LibraryEntry> &entries, bool storageAvailable,
                   size_t totalBytes, size_t usedBytes, uint16_t invalidCount);
  void showSavedDetail(uint32_t id, const AceTagData &tag);
  void showSaveDuplicate();
  void showSaveResult(bool success, uint32_t id, const char *message);
  void showDeleteResult(bool success);
  void showSetup(const char *volumeLabel, uint16_t screenTimeoutSeconds);
  void showEmulationWaiting(const AceTagData &tag, bool fromSaved,
                            bool fromPreset = false);
  void showEmulationProgress(uint8_t startPage);
  bool emulationCancelRequested();
  void showEmulationResult(bool complete, const char *detail);
  void runTouchCalibration() { calibrate(); }
  void updateBattery(uint8_t percent, uint16_t millivolts);
  uint32_t selectedLibraryId() const { return selectedLibraryId_; }
  const AceTagData &selectedSavedTag() const { return savedTag_; }
  const AceTagData &retainedTag() const { return cachedTag_; }
  void setCloneMode(bool enabled) {
    cloneMode_ = enabled;
    if (!enabled) savedCopyMode_ = false;
  }
  const AcePreset &selectedPreset() const { return AcePresets::ALL[selectedPreset_]; }
  bool writeWorkflowActive() const;

 private:
  void calibrate();
  void drawHeader(const char *rightText);
  void drawBatteryStatus();
  void drawAceResult();
  void drawRawInspector();
  void drawFooterButton(int16_t x, int16_t width, const char *label, uint16_t color,
                        uint16_t textColor = TFT_WHITE);
  void drawRemovePrompt();
  void drawWriteSelect();
  void drawWritePreview();
  void drawLibrary();
  void drawSavedDetail();
  void drawStorageInfo();

  BlindRemoteTypefaceTft tft_;
  XPT2046_Touchscreen touch_;
  Preferences preferences_;
  int minX_ = 300;
  int maxX_ = 3800;
  int minY_ = 280;
  int maxY_ = 3850;
  UiScreen screen_ = UiScreen::Scan;
  Pn532Status readerStatus_{};
  AceTagData cachedTag_{};
  bool hasCachedTag_ = false;
  bool tagPresent_ = false;
  bool touchWasDown_ = false;
  bool longPressHandled_ = false;
  uint32_t touchStartedMs_ = 0;
  bool cloneMode_ = false;
  bool savedCopyMode_ = false;
  bool rawIsSaved_ = false;
  int16_t touchStartX_ = 0;
  int16_t touchStartY_ = 0;
  int16_t touchLastX_ = 0;
  int16_t touchLastY_ = 0;
  int16_t rawDragStartY_ = 0;
  uint8_t rawStartOffset_ = 0;
  uint8_t rawDragStartOffset_ = 0;
  uint8_t selectedMaterial_ = 0;
  uint8_t presetScroll_ = 0;
  uint8_t presetDragStartOffset_ = 0;
  int16_t selectedPreset_ = 0;
  String progressPhase_;
  const std::vector<LibraryEntry> *libraryEntries_ = nullptr;
  AceTagData savedTag_{};
  uint32_t selectedLibraryId_ = 0;
  uint16_t libraryScroll_ = 0;
  size_t storageTotal_ = 0;
  size_t storageUsed_ = 0;
  uint16_t invalidFiles_ = 0;
  bool storageAvailable_ = false;
  bool emulationFromSaved_ = false;
  bool emulationFromPreset_ = false;
  bool emulationCancelArmed_ = false;
  uint8_t batteryPercent_ = 0;
  uint16_t batteryMillivolts_ = 0;
};
