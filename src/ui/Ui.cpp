#include "Ui.h"

#include "../config/Hardware.h"
#include "../rfid/AceCodec.h"

namespace {
constexpr uint16_t BG = 0x0841;
constexpr uint16_t HEADER = 0x018C;
constexpr uint16_t TOP_HEADER = 0x0455;
constexpr uint16_t CYAN = 0x07FF;
constexpr uint16_t MUTED = 0xAD55;
constexpr uint16_t AMBER = 0xFFE0;
}  // namespace

Ui::Ui() : touch_(Hardware::TOUCH_CS_PIN, Hardware::TOUCH_IRQ_PIN) {}

void Ui::begin() {
  tft_.init();
  tft_.setRotation(Hardware::SCREEN_ROTATION);
  tft_.setTextWrap(false);
  touch_.begin(TFT_eSPI::getSPIinstance());
  touch_.setRotation(Hardware::SCREEN_ROTATION);
  Serial.println("[display] ST7796 initialized 320x480 rotation=0");
  Serial.println("[touch] XPT2046 initialized CS=33 IRQ=36");
}

void Ui::calibrateIfNeeded() {
  // Open read/write so first boot creates the namespace instead of emitting
  // an expected NVS_NOT_FOUND error before calibration values are saved.
  preferences_.begin("ace-handheld", false);
  const bool calibrated = preferences_.getBool("touchCal", false);
  minX_ = preferences_.getInt("touchMinX", minX_);
  maxX_ = preferences_.getInt("touchMaxX", maxX_);
  minY_ = preferences_.getInt("touchMinY", minY_);
  maxY_ = preferences_.getInt("touchMaxY", maxY_);
  preferences_.end();
  if (!calibrated) calibrate();
  else Serial.println("[touch] Saved calibration loaded");
}

void Ui::drawHeader(const char *rightText) {
  tft_.fillRect(0, 0, Hardware::SCREEN_WIDTH, 54, TOP_HEADER);
  tft_.fillRect(0, 51, Hardware::SCREEN_WIDTH, 3, CYAN);
  tft_.setTextDatum(ML_DATUM);
  tft_.setTextColor(TFT_GREEN, TOP_HEADER);
  tft_.drawString("ACE RFID HANDHELD", 9, 15, 4);
  tft_.setTextColor(AMBER, TOP_HEADER);
  String pageTitle(rightText);
  if (pageTitle.length() > 13) pageTitle = pageTitle.substring(0, 12) + ".";
  tft_.drawString(pageTitle, 9, 38, 4);
  drawBatteryStatus();
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawBatteryStatus() {
  // Battery shares the second header line with the current page status.
  tft_.fillRect(168, 28, 152, 21, TOP_HEADER);
  const uint16_t levelColor = batteryPercent_ > 30 ? TFT_GREEN :
                              batteryPercent_ > 10 ? AMBER : TFT_RED;
  // Match the compact primitive icon used by the SwitchBot Blind Remote.
  constexpr int16_t bx = 176, by = 33;
  tft_.drawRect(bx, by, 18, 11, levelColor);
  tft_.fillRect(bx + 18, by + 3, 3, 5, levelColor);
  const int16_t levelWidth = constrain((14 * batteryPercent_) / 100, 0, 14);
  if (levelWidth > 0) tft_.fillRect(bx + 2, by + 2, levelWidth, 7, levelColor);
  tft_.setTextDatum(MR_DATUM);
  tft_.setTextColor(levelColor, TOP_HEADER);
  tft_.drawString(String(batteryPercent_) + "%", 258, 39, 4);
  tft_.setTextColor(TFT_WHITE, TOP_HEADER);
  tft_.drawString(String(batteryMillivolts_ / 1000.0f, 2) + "V", 316, 39, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::updateBattery(uint8_t percent, uint16_t millivolts) {
  if (percent == batteryPercent_ && millivolts == batteryMillivolts_) return;
  batteryPercent_ = percent;
  batteryMillivolts_ = millivolts;
  drawBatteryStatus();
}

void Ui::showBoot() {
  tft_.fillScreen(BG);
  drawHeader("BOOT");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString("Display: OK", 160, 160, 4);
  tft_.drawString("Touch: OK", 160, 210, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Searching for PN532...", 160, 285, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showReaderStatus(const Pn532Status &status) {
  readerStatus_ = status;
  screen_ = UiScreen::Scan;
  tft_.fillScreen(BG);
  drawHeader(status.found && status.samConfigured ? "READY" : "ERROR");
  tft_.setTextDatum(MC_DATUM);
  if (!status.found) {
    tft_.setTextColor(TFT_RED, BG);
    tft_.drawString("PN532 NOT FOUND", 160, 175, 4);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("Check SPI mode and wiring", 160, 225, 2);
    tft_.TFT_eSPI::drawString("Restart after correcting wiring", 160, 255, 2);
  } else if (!status.samConfigured) {
    tft_.setTextColor(TFT_RED, BG);
    tft_.drawString("SAM CONFIG FAILED", 160, 190, 4);
  } else {
    // Code-drawn RFID mark based on the supplied reference: a square open
    // card, transmitter dot, and three waves matching the card stroke width.
    constexpr uint16_t RFID_BLUE = 0x5B3F;
    for (int16_t inset = 0; inset < 6; ++inset) {
      tft_.drawRoundRect(100 + inset, 105 + inset, 120 - inset * 2,
                         120 - inset * 2, 9 - inset, RFID_BLUE);
    }
    tft_.fillRect(118, 101, 106, 16, BG);
    // Keep the upper-right card edge below the radio waves so the two marks
    // remain visually separate.
    tft_.fillRect(214, 101, 10, 90, BG);
    tft_.fillCircle(175, 133, 11, RFID_BLUE);
    tft_.drawArc(175, 133, 27, 21, 125, 335, RFID_BLUE, BG);
    tft_.drawArc(175, 133, 43, 37, 125, 335, RFID_BLUE, BG);
    tft_.drawArc(175, 133, 59, 53, 125, 335, RFID_BLUE, BG);
    tft_.setTextColor(RFID_BLUE, BG);
    tft_.drawString("READY", 145, 207, 4);
    tft_.setTextColor(TFT_WHITE, BG);
    tft_.drawString("PRESENT RFID TAG", 160, 285, 4);
    // Use TFT_eSPI Font 2 for secondary status text. It is smoother and about
    // twice the height of the small classic face while both lines still fit.
    tft_.setTextFont(2);
    tft_.setTextSize(1);
    tft_.setTextColor(TFT_GREEN, BG);
    tft_.TFT_eSPI::drawString("PN532 ONLINE", 160, 325, 2);
    tft_.setTextColor(AMBER, BG);
    tft_.TFT_eSPI::drawString("Hold tag near antenna on back", 160, 355, 2);
    drawFooterButton(0, 156, "LIBRARY", 0x2124);
    drawFooterButton(160, 160, "WRITE PRESET", 0x2124);
    // High-contrast outlines make the two home actions read as buttons.
    tft_.drawRoundRect(3, 431, 150, 46, 6, CYAN);
    tft_.drawRoundRect(163, 431, 154, 46, 6, CYAN);
  }
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showUid(const String &uid) {
  tft_.fillScreen(BG);
  drawHeader("TAG FOUND");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(CYAN, BG);
  tft_.drawString("TAG FOUND", 160, 145, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("UID", 160, 215, 2);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(uid, 160, 255, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Remove tag to scan again", 160, 330, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showAceTag(const AceTagData &tag) {
  cachedTag_ = tag;
  hasCachedTag_ = true;
  tagPresent_ = true;
  screen_ = UiScreen::AceResult;
  drawAceResult();
  Serial.println("[ui] Screen=ACE_RESULT");
}

void Ui::drawAceResult() {
  const AceTagData &tag = cachedTag_;
  tft_.fillScreen(BG);
  drawHeader("ACE TAG");

  const uint16_t plate = AceCodec::rgb565(tag);
  const uint16_t text = AceCodec::contrastText(tag);
  constexpr int16_t px = 20, py = 60, pw = 280, ph = 105;
  tft_.fillRoundRect(px, py, pw, ph, 12, plate);
  // Double border remains visible for white, clear, and very pale colors.
  tft_.drawRoundRect(px, py, pw, ph, 12, TFT_WHITE);
  tft_.drawRoundRect(px + 1, py + 1, pw - 2, ph - 2, 11, TFT_DARKGREY);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(text, plate);
  String colorLabel(tag.colorName);
  colorLabel.toUpperCase();
  if (colorLabel.length() > 13) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.TFT_eSPI::drawString(colorLabel, 160, 112, 2);
  } else tft_.drawString(colorLabel, 160, 112, 4);

  tft_.setTextColor(CYAN, BG);
  tft_.drawString(tag.material, 160, 185, 4);
  tft_.setTextDatum(TL_DATUM);

  auto row = [this](int16_t y, const char *label, const String &value) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString(label, 20, y, 2);
    tft_.setTextDatum(TR_DATUM);
    tft_.setTextColor(TFT_WHITE, BG);
    tft_.TFT_eSPI::drawString(value, 300, y, 2);
    tft_.setTextDatum(TL_DATUM);
  };
  row(210, "SKU", tag.sku[0] ? String(tag.sku) : String("-"));
  row(238, "Nozzle", String(tag.nozzleMin) + " - " + String(tag.nozzleMax) + " C");
  row(266, "Bed", String(tag.bedMin) + " - " + String(tag.bedMax) + " C");
  row(294, "Diameter", String(tag.diameterHundredthsMm / 100.0f, 2) + " mm");
  row(322, "Length", String(tag.lengthMeters) + " m");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(MUTED, BG);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.TFT_eSPI::drawString("UID " + tag.uidText, 160, 352, 2);
  tft_.setTextDatum(TL_DATUM);

  auto actionButton = [this](int16_t x, int16_t width, const char *label,
                             uint16_t fill, uint16_t textColor) {
    tft_.fillRect(x, 376, width, 52, fill);
    tft_.drawRect(x, 376, width, 52, MUTED);
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(textColor, fill);
    tft_.drawString(label, x + width / 2, 402, 4);
    tft_.setTextDatum(TL_DATUM);
  };
  actionButton(0, 60, "RAW", HEADER, TFT_WHITE);
  actionButton(64, 60, "CLONE", CYAN, TFT_BLACK);
  actionButton(128, 60, "SAVE", 0x0400, TFT_WHITE);
  actionButton(192, 60, "EMU", AMBER, TFT_BLACK);
  actionButton(256, 64, "WRITE", HEADER, TFT_WHITE);
  drawFooterButton(0, 320, "HOME", 0x2124);
}

void Ui::showGenericTag(const AceTagData &tag) {
  cachedTag_ = tag;
  hasCachedTag_ = true;
  tagPresent_ = true;
  screen_ = UiScreen::GenericTag;
  tft_.fillScreen(BG);
  drawHeader("NFC TAG");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(CYAN, BG);
  tft_.drawString("TAG FOUND", 160, 135, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString("Not recognized as an", 160, 205, 2);
  tft_.drawString("ACE filament tag", 160, 235, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("UID", 160, 300, 2);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString(tag.uidText, 160, 335, 2);
  drawRemovePrompt();
  drawFooterButton(0, 156, "RAW DATA", HEADER);
  drawFooterButton(160, 160, "WRITE PRESET", HEADER);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=GENERIC_TAG");
}

void Ui::showReadError(const AceTagData &tag) {
  cachedTag_ = tag;
  screen_ = UiScreen::ReadError;
  tft_.fillScreen(BG);
  drawHeader("READ ERROR");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_RED, BG);
  tft_.drawString("MEMORY READ FAILED", 160, 175, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString("UID was detected", 160, 235, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Could not read required memory", 160, 275, 2);
  if (tag.failedPage >= 0)
    tft_.TFT_eSPI::drawString("Failed page: " + String(tag.failedPage), 160, 303, 2);
  tft_.TFT_eSPI::drawString(tag.uidText, 160, 330, 2);
  tft_.TFT_eSPI::drawString("REMOVE TAG TO RETRY", 160, 430, 2);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=READ_ERROR");
}

void Ui::showUnsupportedTag(const AceTagData &tag) {
  cachedTag_ = tag;
  screen_ = UiScreen::UnsupportedTag;
  tft_.fillScreen(BG);
  drawHeader("UNSUPPORTED");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(AMBER, BG);
  tft_.drawString("TAG DETECTED", 160, 145, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString("Unsupported tag type", 160, 215, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.TFT_eSPI::drawString("or memory layout", 160, 250, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("UID", 160, 305, 2);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString(tag.uidText, 160, 340, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("REMOVE TAG TO CONTINUE", 160, 430, 2);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=UNSUPPORTED_TAG");
}

void Ui::drawFooterButton(int16_t x, int16_t width, const char *label, uint16_t color,
                          uint16_t textColor) {
  tft_.fillRect(x, 428, width, 52, color);
  tft_.drawRect(x, 428, width, 52, MUTED);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(textColor, color);
  tft_.drawString(label, x + width / 2, 454, 4);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawRemovePrompt() {
  // Status text for non-ACE retained results.
  tft_.fillRect(0, 402, 320, 25, BG);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(tagPresent_ ? MUTED : CYAN, BG);
  tft_.TFT_eSPI::drawString(tagPresent_ ? "Remove tag to scan another"
                                       : "Tag removed - ready for next tag", 160, 414, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawRawInspector() {
  const AceTagData &rawTag = rawIsSaved_ ? savedTag_ : cachedTag_;
  tft_.fillScreen(BG);
  drawHeader(rawIsSaved_ ? "SAVED RAW" : "RAW DATA");
  tft_.setTextColor(MUTED, BG);
  tft_.drawString("UID " + rawTag.uidText, 12, 61, 2);
  tft_.setTextColor(CYAN, BG);
  tft_.drawString("PAGE", 18, 86, 2);
  tft_.drawString("DATA (4 BYTES)", 100, 86, 2);
  tft_.drawFastHLine(12, 106, 296, HEADER);
  constexpr uint8_t visibleRows = 11;
  for (uint8_t row = 0; row < visibleRows; ++row) {
    const uint8_t page = 4 + rawStartOffset_ + row;
    if (page > 31) break;
    const int16_t y = 112 + row * 27;
    char pageText[4];
    char dataText[16];
    snprintf(pageText, sizeof(pageText), "%02u", page);
    snprintf(dataText, sizeof(dataText), "%02X  %02X  %02X  %02X",
             rawTag.pages[page][0], rawTag.pages[page][1],
             rawTag.pages[page][2], rawTag.pages[page][3]);
    tft_.setTextColor(TFT_WHITE, BG);
    tft_.drawString(pageText, 24, y, 2);
    tft_.drawString(dataText, 100, y, 2);
  }
  drawFooterButton(0, 320, "BACK", HEADER);
}

UiTouchSample Ui::sampleTouch() {
  UiTouchSample sample;
  sample.down = touch_.touched();
  if (!sample.down) return sample;
  const TS_Point raw = touch_.getPoint();
  sample.x = constrain(map(raw.x, minX_, maxX_, 0, Hardware::SCREEN_WIDTH - 1),
                       0, Hardware::SCREEN_WIDTH - 1);
  sample.y = constrain(map(raw.y, minY_, maxY_, 0, Hardware::SCREEN_HEIGHT - 1),
                       0, Hardware::SCREEN_HEIGHT - 1);
  return sample;
}

UiAction Ui::handleTouch(const UiTouchSample &sample, bool controlsAllowed) {
  if (!controlsAllowed) {
    touchWasDown_ = false;
    return UiAction::None;
  }
  if (sample.down && !touchWasDown_) {
    touchWasDown_ = true;
    longPressHandled_ = false;
    touchStartedMs_ = millis();
    touchStartX_ = sample.x;
    touchStartY_ = sample.y;
    touchLastX_ = sample.x;
    touchLastY_ = sample.y;
    rawDragStartY_ = sample.y;
    rawDragStartOffset_ = rawStartOffset_;
    presetDragStartOffset_ = presetScroll_;
    return UiAction::None;
  }
  if (sample.down && touchWasDown_ && screen_ == UiScreen::Scan &&
      !longPressHandled_ && millis() - touchStartedMs_ >= 1200 &&
      abs(sample.x - touchStartX_) < 24 && abs(sample.y - touchStartY_) < 24) {
    longPressHandled_ = true;
    Serial.println("[ui] Scan long press -> SETUP");
    return UiAction::OpenSetup;
  }
  if (sample.down && touchWasDown_ &&
      (screen_ == UiScreen::RawInspector || screen_ == UiScreen::SavedRaw ||
       screen_ == UiScreen::WriteSelect || screen_ == UiScreen::LibraryList)) {
    touchLastX_ = sample.x;
    touchLastY_ = sample.y;
    if (screen_ == UiScreen::RawInspector || screen_ == UiScreen::SavedRaw) {
      const int16_t deltaRows = (rawDragStartY_ - sample.y) / 24;
      const uint8_t next = constrain(static_cast<int>(rawDragStartOffset_) + deltaRows, 0, 17);
      if (next != rawStartOffset_) {
        rawStartOffset_ = next; drawRawInspector();
        Serial.printf("[ui] Raw scroll offset=%u\n", rawStartOffset_);
      }
    }
    // Preset gestures are only rendered after release. Redrawing the TFT
    // while XPT2046 is actively sampled made swipes feel intermittent.
    return UiAction::None;
  }
  if (sample.down && touchWasDown_) {
    touchLastX_ = sample.x;
    touchLastY_ = sample.y;
    return UiAction::None;
  }
  if (sample.down || !touchWasDown_) return UiAction::None;
  touchWasDown_ = false;
  if (longPressHandled_) {
    longPressHandled_ = false;
    return UiAction::None;
  }
  // The right edge contains exclusive page buttons. A touch that begins
  // here can never select a preset.
  if (screen_ == UiScreen::WriteSelect && touchStartX_ >= 252 &&
      touchStartY_ >= 132 && touchStartY_ < 404) {
    const int count = AcePresets::countForMaterial(selectedMaterial_);
    const int maxOffset = max(0, count - 6);
    const bool buttonTap = abs(touchLastX_ - touchStartX_) < 30 &&
                           abs(touchLastY_ - touchStartY_) < 30;
    uint8_t nextOffset = presetScroll_;
    if (buttonTap && touchStartY_ < 266) {
      nextOffset = max(0, static_cast<int>(presetScroll_) - 6);
    } else if (buttonTap && touchStartY_ >= 270) {
      nextOffset = min(maxOffset, static_cast<int>(presetScroll_) + 6);
    }
    if (nextOffset != presetScroll_) {
      presetScroll_ = nextOffset;
      drawWriteSelect();
      Serial.printf("[ui] Preset page offset=%u\n", presetScroll_);
    }
    return UiAction::None;
  }
  // Library paging uses the same exclusive 64-pixel right-side controls as
  // the preset selector. Touches here can never open a saved record.
  if (screen_ == UiScreen::LibraryList && touchStartX_ >= 252 &&
      touchStartY_ >= 64 && touchStartY_ < 414) {
    const int count = libraryEntries_ ? libraryEntries_->size() : 0;
    const int maxOffset = max(0, count - 5);
    const bool buttonTap = abs(touchLastX_ - touchStartX_) < 30 &&
                           abs(touchLastY_ - touchStartY_) < 30;
    uint16_t nextOffset = libraryScroll_;
    if (buttonTap && touchStartY_ < 235)
      nextOffset = max(0, static_cast<int>(libraryScroll_) - 5);
    else if (buttonTap && touchStartY_ >= 239)
      nextOffset = min(maxOffset, static_cast<int>(libraryScroll_) + 5);
    if (nextOffset != libraryScroll_) {
      libraryScroll_ = nextOffset;
      drawLibrary();
      Serial.printf("[ui] Library page offset=%u\n", libraryScroll_);
    }
    return UiAction::None;
  }
  const bool tap = abs(touchLastX_ - touchStartX_) < 30 &&
                   abs(touchLastY_ - touchStartY_) < 30;
  if (!tap) {
    return UiAction::None;
  }
  if (screen_ == UiScreen::Scan && touchStartY_ >= 420) {
    if (touchStartX_ < 158) return UiAction::OpenLibrary;
    showWriteSelect();
  } else if (screen_ == UiScreen::WriteSelect) {
    if (touchStartY_ >= 62 && touchStartY_ <= 112) {
      const int16_t tabWidth = Hardware::SCREEN_WIDTH / AcePresets::MATERIAL_COUNT;
      selectedMaterial_ = constrain(touchStartX_ / tabWidth, 0, AcePresets::MATERIAL_COUNT - 1);
      presetScroll_ = 0; drawWriteSelect();
    } else if (touchStartX_ < 252 && touchStartY_ >= 136 && touchStartY_ < 400) {
      const uint8_t row = (touchStartY_ - 136) / 44;
      const int16_t index = AcePresets::globalIndex(selectedMaterial_, presetScroll_ + row);
      if (index >= 0) { selectedPreset_ = index; screen_ = UiScreen::WritePreview; drawWritePreview(); }
    } else if (touchStartY_ >= 420) showReaderStatus(readerStatus_);
  } else if (screen_ == UiScreen::WritePreview && touchStartY_ >= 420) {
    if (touchStartX_ < 158) showWriteSelect();
    else { showWriteWaiting(true); return UiAction::WriteArmed; }
  } else if ((screen_ == UiScreen::WriteWaiting || screen_ == UiScreen::CloneCaptured ||
              screen_ == UiScreen::CloneWaiting) && touchStartY_ >= 420) {
    if (savedCopyMode_) { cloneMode_ = false; savedCopyMode_ = false; screen_ = UiScreen::SavedDetail; drawSavedDetail(); }
    else if (cloneMode_) restoreRetainedAce(); else showReaderStatus(readerStatus_);
    return UiAction::WriteCancel;
  } else if (screen_ == UiScreen::WriteFailure && touchStartY_ >= 420) {
    if (touchStartX_ < 158) {
      if (savedCopyMode_) showSavedCopyCaptured(savedTag_);
      else if (cloneMode_) showCloneCaptured(cachedTag_); else showWriteWaiting(true);
      return UiAction::WriteRetry;
    }
    if (savedCopyMode_) { cloneMode_ = false; savedCopyMode_ = false; screen_ = UiScreen::SavedDetail; drawSavedDetail(); }
    else if (cloneMode_) restoreRetainedAce(); else showReaderStatus(readerStatus_);
    return UiAction::WriteCancel;
  } else if ((screen_ == UiScreen::WriteSuccess || screen_ == UiScreen::CloneSuccess) &&
             touchStartY_ >= 420) {
    showReaderStatus(readerStatus_); return UiAction::WriteDone;
  }
  else if (screen_ == UiScreen::AceResult && touchStartY_ >= 428) {
    showReaderStatus(readerStatus_);
    Serial.println("[ui] HOME -> SCAN");
  }
  else if (screen_ == UiScreen::AceResult && touchStartY_ >= 376 &&
           touchStartY_ < 428 && hasCachedTag_) {
    if (touchStartX_ < 62) {
      rawIsSaved_ = false;
      screen_ = UiScreen::RawInspector;
      rawStartOffset_ = 0;
      drawRawInspector();
      Serial.println("[ui] Screen=RAW_INSPECTOR");
    } else if (touchStartX_ < 126) {
      return UiAction::CloneStart;
    } else if (touchStartX_ < 190) {
      return UiAction::SaveTag;
    } else if (touchStartX_ < 254) {
      return UiAction::EmulatePresent;
    } else showWriteSelect();
  }
  else if (screen_ == UiScreen::GenericTag &&
      touchStartY_ >= 420 && hasCachedTag_) {
    if (touchStartX_ < 158) {
      screen_ = UiScreen::RawInspector;
      rawStartOffset_ = 0;
      drawRawInspector();
      Serial.println("[ui] Screen=RAW_INSPECTOR");
    } else showWriteSelect();
  } else if (screen_ == UiScreen::RawInspector && touchStartY_ >= 420) {
    screen_ = cachedTag_.aceValid ? UiScreen::AceResult : UiScreen::GenericTag;
    if (cachedTag_.aceValid) drawAceResult(); else showGenericTag(cachedTag_);
    Serial.println(cachedTag_.aceValid ? "[ui] Screen=ACE_RESULT" : "[ui] Screen=GENERIC_TAG");
  } else if (screen_ == UiScreen::LibraryList) {
    if (touchStartY_ >= 420) {
      if (touchStartX_ < 158) { screen_ = UiScreen::StorageInfo; drawStorageInfo(); }
      else showReaderStatus(readerStatus_);
    } else if (touchStartX_ < 252 && touchStartY_ >= 64 &&
               touchStartY_ < 414 && libraryEntries_) {
      const uint16_t row = (touchStartY_ - 64) / 70;
      const uint16_t index = libraryScroll_ + row;
      if (index < libraryEntries_->size()) {
        selectedLibraryId_ = (*libraryEntries_)[index].id;
        return UiAction::OpenSaved;
      }
    }
  } else if (screen_ == UiScreen::SavedDetail && touchStartY_ >= 428) {
    return UiAction::RefreshLibrary;
  } else if (screen_ == UiScreen::SavedDetail && touchStartY_ >= 376) {
    if (touchStartX_ < 78) {
      rawIsSaved_ = true; rawStartOffset_ = 0; screen_ = UiScreen::SavedRaw;
      drawRawInspector();
    } else if (touchStartX_ < 158) return UiAction::EmulateSaved;
    else if (touchStartX_ < 238) return UiAction::WriteSaved;
    else {
      screen_ = UiScreen::DeleteConfirm;
      tft_.fillScreen(BG); drawHeader("CONFIRM DELETE");
      tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
      tft_.drawString("DELETE SAVED TAG?", 160, 145, 4);
      tft_.setTextColor(TFT_WHITE, BG);
      tft_.drawString(String(savedTag_.material) + " " + savedTag_.colorName, 160, 220, 4);
      tft_.setTextFont(2); tft_.setTextSize(1);
      tft_.setTextColor(AMBER, BG);
      tft_.TFT_eSPI::drawString("This cannot be undone", 160, 300, 2);
      drawFooterButton(0, 156, "DELETE", TFT_RED);
      drawFooterButton(160, 160, "CANCEL", 0x2124);
      tft_.setTextDatum(TL_DATUM);
    }
  } else if (screen_ == UiScreen::SavedRaw && touchStartY_ >= 420) {
    screen_ = UiScreen::SavedDetail; drawSavedDetail();
  } else if (screen_ == UiScreen::DeleteConfirm && touchStartY_ >= 420) {
    if (touchStartX_ < 158) return UiAction::ConfirmDelete;
    screen_ = UiScreen::SavedDetail; drawSavedDetail();
  } else if (screen_ == UiScreen::SaveDuplicate && touchStartY_ >= 420) {
    if (touchStartX_ < 158) return UiAction::SaveCopy;
    restoreRetainedAce();
  } else if (screen_ == UiScreen::SaveResult && touchStartY_ >= 420) {
    restoreRetainedAce();
  } else if (screen_ == UiScreen::StorageInfo && touchStartY_ >= 420) {
    if (!storageAvailable_ && touchStartX_ >= 160) {
      screen_ = UiScreen::FormatConfirm;
      tft_.fillScreen(BG); drawHeader("CONFIRM FORMAT");
      tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
      tft_.drawString("FORMAT STORAGE?", 160, 145, 4);
      tft_.setTextFont(2); tft_.setTextSize(1);
      tft_.setTextColor(AMBER, BG);
      tft_.TFT_eSPI::drawString("All saved tags will be erased", 160, 225, 2);
      tft_.setTextColor(TFT_WHITE, BG);
      tft_.TFT_eSPI::drawString("Use only for first-time setup", 160, 280, 2);
      drawFooterButton(0, 156, "FORMAT", TFT_RED);
      drawFooterButton(160, 160, "CANCEL", 0x2124);
      tft_.setTextDatum(TL_DATUM);
      return UiAction::None;
    }
    drawLibrary(); screen_ = UiScreen::LibraryList;
  } else if (screen_ == UiScreen::FormatConfirm && touchStartY_ >= 420) {
    if (touchStartX_ < 158) return UiAction::FormatStorage;
    screen_ = UiScreen::StorageInfo; drawStorageInfo();
  } else if (screen_ == UiScreen::EraseConfirm && touchStartY_ >= 420) {
    if (touchStartX_ < 158) return UiAction::FormatStorage;
    return UiAction::OpenSetup;
  } else if (screen_ == UiScreen::Setup) {
    if (touchStartY_ >= 60 && touchStartY_ < 120) return UiAction::CalibrateTouch;
    if (touchStartY_ >= 126 && touchStartY_ < 186) return UiAction::CycleVolume;
    if (touchStartY_ >= 192 && touchStartY_ < 252) return UiAction::CycleScreenTimeout;
    if (touchStartY_ >= 258 && touchStartY_ < 318) {
      screen_ = UiScreen::EraseConfirm;
      tft_.fillScreen(BG); drawHeader("CONFIRM ERASE");
      tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
      tft_.drawString("ERASE LIBRARY?", 160, 145, 4);
      tft_.setTextFont(2); tft_.setTextSize(1);
      tft_.setTextColor(AMBER, BG);
      tft_.TFT_eSPI::drawString("All saved tags will be deleted", 160, 225, 2);
      tft_.setTextColor(TFT_WHITE, BG);
      tft_.TFT_eSPI::drawString("RFID tags are not affected", 160, 275, 2);
      tft_.TFT_eSPI::drawString("This cannot be undone", 160, 325, 2);
      drawFooterButton(0, 156, "ERASE", TFT_RED);
      drawFooterButton(160, 160, "CANCEL", 0x2124);
      tft_.setTextDatum(TL_DATUM);
      return UiAction::None;
    }
    if (touchStartY_ >= 420) return UiAction::CloseSetup;
  } else if (screen_ == UiScreen::EmulateResult && touchStartY_ >= 420) {
    if (emulationFromSaved_) { screen_ = UiScreen::SavedDetail; drawSavedDetail(); }
    else restoreRetainedAce();
  }
  return UiAction::None;
}

void Ui::onFieldCleared() {
  if (screen_ == UiScreen::RawInspector || screen_ == UiScreen::SavedRaw ||
      screen_ == UiScreen::LibraryList || screen_ == UiScreen::SavedDetail ||
      screen_ == UiScreen::DeleteConfirm || screen_ == UiScreen::StorageInfo ||
      screen_ == UiScreen::Setup || screen_ == UiScreen::Scan) return;
  tagPresent_ = false;
  if (screen_ == UiScreen::AceResult) {
    drawAceResult();
    Serial.println("[ui] Tag removed; ACE result retained");
    return;
  }
  if (screen_ == UiScreen::GenericTag) {
    showGenericTag(cachedTag_);
    tagPresent_ = false;
    drawRemovePrompt();
    Serial.println("[ui] Tag removed; generic result retained");
    return;
  }
  showReaderStatus(readerStatus_);
  Serial.println("[ui] Screen=SCAN");
}

bool Ui::writeWorkflowActive() const {
  return screen_ == UiScreen::WriteSelect || screen_ == UiScreen::WritePreview ||
         screen_ == UiScreen::WriteWaiting || screen_ == UiScreen::WriteProgress ||
         screen_ == UiScreen::WriteSuccess || screen_ == UiScreen::WriteFailure ||
         screen_ == UiScreen::CloneCaptured || screen_ == UiScreen::CloneWaiting ||
         screen_ == UiScreen::CloneSuccess || screen_ == UiScreen::Setup ||
         screen_ == UiScreen::EmulateWaiting || screen_ == UiScreen::EmulateResult;
}

void Ui::showWriteSelect() {
  screen_ = UiScreen::WriteSelect;
  drawWriteSelect();
  Serial.println("[ui] Screen=WRITE_SELECT");
}

void Ui::drawWriteSelect() {
  tft_.fillScreen(BG);
  drawHeader("WRITE TAG");
  for (uint8_t i = 0; i < AcePresets::MATERIAL_COUNT; ++i) {
    const int16_t tabWidth = Hardware::SCREEN_WIDTH / AcePresets::MATERIAL_COUNT;
    const int16_t x = i * tabWidth;
    const uint16_t fill = i == selectedMaterial_ ? TFT_GREEN : HEADER;
    tft_.fillRect(x + 2, 64, tabWidth - 4, 46, fill);
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextColor(i == selectedMaterial_ ? TFT_BLACK : TFT_WHITE, fill);
    tft_.drawString(AcePresets::MATERIALS[i], x + tabWidth / 2, 87, 4);
  }
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(MUTED, BG);
  tft_.drawString("Tap a color", 12, 118, 2);
  const uint8_t materialCount = AcePresets::countForMaterial(selectedMaterial_);
  tft_.setTextDatum(TR_DATUM);
  tft_.drawString(String(presetScroll_ + 1) + "-" +
                  String(min(static_cast<int>(materialCount),
                             static_cast<int>(presetScroll_ + 6))) + " / " +
                  String(materialCount), 246, 118, 2);
  tft_.setTextDatum(TL_DATUM);
  for (uint8_t row = 0; row < 6; ++row) {
    const uint8_t filtered = presetScroll_ + row;
    const int16_t index = AcePresets::globalIndex(selectedMaterial_, filtered);
    if (index < 0) break;
    const int16_t y = 136 + row * 44;
    tft_.drawRect(8, y, 240, 40, HEADER);
    AceTagData swatchTag;
    AcePresets::buildTag(AcePresets::ALL[index], swatchTag);
    AceCodec::validateAndDecode(swatchTag);
    const uint16_t swatch = AceCodec::rgb565(swatchTag);
    tft_.fillRoundRect(14, y + 5, 30, 30, 5, swatch);
    // A double outline keeps white, clear, yellow, and pale swatches visible.
    tft_.drawRoundRect(14, y + 5, 30, 30, 5, TFT_WHITE);
    tft_.drawRoundRect(15, y + 6, 28, 28, 4, TFT_DARKGREY);
    tft_.setTextColor(TFT_WHITE, BG);
    char number[7];
    snprintf(number, sizeof(number), "#%02u", filtered + 1);
    tft_.setTextColor(MUTED, BG);
    tft_.drawString(number, 50, y + 2, 2);
    tft_.setTextColor(TFT_WHITE, BG);
    String rowName = selectedMaterial_ == AcePresets::MATERIAL_COUNT - 1
                         ? String(AcePresets::ALL[index].name)
                         : String(swatchTag.colorName);
    if (selectedMaterial_ == AcePresets::MATERIAL_COUNT - 1) {
      if (rowName.startsWith("PLA Silk ")) rowName.remove(0, 4);
      if (rowName.endsWith(" Catalog")) rowName.remove(rowName.length() - 8);
    }
    if (rowName.length() > 16) rowName = rowName.substring(0, 15) + ".";
    tft_.drawString(rowName, 50, y + 19, 4);
  }
  // Simple page controls, kept outside all selectable color-row hitboxes.
  const bool hasPrevious = presetScroll_ > 0;
  const bool hasNext = presetScroll_ + 6 < materialCount;
  const uint16_t previousColor = hasPrevious ? CYAN : HEADER;
  const uint16_t nextColor = hasNext ? CYAN : HEADER;
  tft_.fillRoundRect(256, 136, 64, 126, 7, previousColor);
  tft_.drawRoundRect(256, 136, 64, 126, 7, MUTED);
  tft_.fillRoundRect(256, 270, 64, 130, 7, nextColor);
  tft_.drawRoundRect(256, 270, 64, 130, 7, MUTED);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(hasPrevious ? TFT_BLACK : MUTED, previousColor);
  tft_.drawString("PREV", 288, 199, 4);
  tft_.setTextColor(hasNext ? TFT_BLACK : MUTED, nextColor);
  tft_.drawString("NEXT", 288, 335, 4);
  tft_.setTextDatum(TL_DATUM);
  drawFooterButton(0, 320, "CANCEL", 0x2124);
}

void Ui::drawWritePreview() {
  AceTagData preview;
  AcePresets::buildTag(selectedPreset(), preview);
  AceCodec::validateAndDecode(preview);
  tft_.fillScreen(BG);
  drawHeader(selectedPreset().factoryVerified ? "VERIFIED" :
             selectedPreset().catalogDerived ? "CATALOG" : "COMMUNITY");
  const uint16_t plate = AceCodec::rgb565(preview);
  const uint16_t text = AceCodec::contrastText(preview);
  tft_.fillRoundRect(20, 65, 280, 115, 12, plate);
  tft_.drawRoundRect(20, 65, 280, 115, 12, TFT_WHITE);
  tft_.drawRoundRect(21, 66, 278, 113, 11, TFT_DARKGREY);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(text, plate);
  String color(selectedPreset().colorName); color.toUpperCase();
  if (color.length() > 13) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.TFT_eSPI::drawString(color, 160, 122, 2);
  } else tft_.drawString(color, 160, 122, 4);
  tft_.setTextColor(CYAN, BG);
  tft_.drawString(selectedPreset().material, 160, 205, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.TFT_eSPI::drawString(selectedPreset().name, 160, 238, 2);
  tft_.setTextDatum(TL_DATUM);
  auto row = [this](int16_t y, const char *label, const String &value) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG); tft_.TFT_eSPI::drawString(label, 20, y, 2);
    tft_.setTextDatum(TR_DATUM); tft_.setTextColor(TFT_WHITE, BG);
    tft_.TFT_eSPI::drawString(value, 300, y, 2); tft_.setTextDatum(TL_DATUM);
  };
  row(270, "SKU", selectedPreset().sku);
  row(298, "Nozzle", String(selectedPreset().nozzleMin) + " - " + selectedPreset().nozzleMax + " C");
  row(326, "Bed", String(selectedPreset().bedMin) + " - " + selectedPreset().bedMax + " C");
  row(354, "Diameter", String(selectedPreset().diameter / 100.0f, 2) + " mm");
  row(382, "Length", String(selectedPreset().lengthMeters) + " m");
  drawFooterButton(0, 156, "BACK", 0x2124);
  drawFooterButton(160, 160, "WRITE TAG", HEADER);
}

void Ui::showWriteWaiting(bool fieldMustClear) {
  screen_ = UiScreen::WriteWaiting;
  tft_.fillScreen(BG); drawHeader("WRITE ARMED");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(CYAN, BG); tft_.drawString("READY TO WRITE", 160, 135, 4);
  tft_.setTextColor(TFT_WHITE, BG); tft_.drawString(selectedPreset().name, 160, 205, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString(fieldMustClear ? "Remove all tags first" : "Present writable tag", 160, 280, 2);
  tft_.TFT_eSPI::drawString(fieldMustClear ? "Waiting for clear field" : "Hold tag steady", 160, 312, 2);
  drawFooterButton(0, 320, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showWriteProgress(const char *phase, uint8_t completed, uint8_t total) {
  const bool newPhase = screen_ != UiScreen::WriteProgress || progressPhase_ != phase;
  screen_ = UiScreen::WriteProgress;
  if (newPhase) {
    progressPhase_ = phase;
    tft_.fillScreen(BG); drawHeader(phase);
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(CYAN, BG);
    tft_.drawString(phase, 160, 145, 4);
    tft_.drawRect(20, 285, 280, 28, MUTED);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("Keep the same tag in place", 160, 360, 2);
  }
  tft_.fillRect(20, 200, 280, 52, BG);
  tft_.fillRect(21, 286, 278, 26, BG);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(String(completed) + " of " + total + " pages", 160, 225, 4);
  if (total) tft_.fillRect(23, 288, (274UL * completed) / total, 22, CYAN);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showWriteSuccess(const String &uid) {
  screen_ = UiScreen::WriteSuccess;
  tft_.fillScreen(BG); drawHeader("WRITE COMPLETE");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_GREEN, BG);
  tft_.drawString("WRITE COMPLETE", 160, 130, 4);
  tft_.setTextColor(TFT_WHITE, BG); tft_.drawString(selectedPreset().name, 160, 205, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_GREEN, BG);
  tft_.TFT_eSPI::drawString("Write     PASS", 160, 270, 2);
  tft_.TFT_eSPI::drawString("Verify    PASS", 160, 305, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("UID " + uid, 160, 355, 2);
  drawFooterButton(0, 320, "DONE", HEADER); tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=WRITE_SUCCESS");
}

void Ui::showWriteFailure(const char *title, const char *detail, bool partial) {
  screen_ = UiScreen::WriteFailure;
  tft_.fillScreen(BG); drawHeader(cloneMode_ ? "CLONE FAILED" : "WRITE FAILED");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
  tft_.drawString(title, 160, 145, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString(detail, 160, 220, 2);
  if (partial) {
    tft_.setTextColor(AMBER, BG);
    tft_.TFT_eSPI::drawString("TAG MAY CONTAIN", 160, 285, 2);
    tft_.TFT_eSPI::drawString("PARTIAL ACE DATA", 160, 315, 2);
  }
  drawFooterButton(0, 156, "RETRY", HEADER);
  drawFooterButton(160, 160, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
  Serial.printf("[ui] Screen=WRITE_FAILURE reason=%s\n", title);
}

void Ui::showCloneCaptured(const AceTagData &source) {
  cloneMode_ = true;
  savedCopyMode_ = false;
  screen_ = UiScreen::CloneCaptured;
  tft_.fillScreen(BG); drawHeader("SOURCE CAPTURED");
  const uint16_t plate = AceCodec::rgb565(source);
  const uint16_t text = AceCodec::contrastText(source);
  tft_.fillRoundRect(20, 65, 280, 115, 12, plate);
  tft_.drawRoundRect(20, 65, 280, 115, 12, TFT_WHITE);
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(text, plate);
  String color(source.colorName); color.toUpperCase();
  if (color.length() > 13) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.TFT_eSPI::drawString(color, 160, 122, 2);
  } else tft_.drawString(color, 160, 122, 4);
  tft_.setTextColor(CYAN, BG); tft_.drawString(source.material, 160, 210, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG); tft_.TFT_eSPI::drawString(source.sku, 160, 250, 2);
  tft_.setTextColor(AMBER, BG); tft_.drawString("REMOVE SOURCE TAG", 160, 315, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Waiting for clear field", 160, 360, 2);
  drawFooterButton(0, 320, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=CLONE_SOURCE_CAPTURED");
}

void Ui::showSavedCopyCaptured(const AceTagData &source) {
  cloneMode_ = true;
  savedCopyMode_ = true;
  screen_ = UiScreen::CloneCaptured;
  tft_.fillScreen(BG); drawHeader("SAVED COPY");
  const uint16_t plate = AceCodec::rgb565(source);
  const uint16_t text = AceCodec::contrastText(source);
  tft_.fillRoundRect(20, 65, 280, 115, 12, plate);
  tft_.drawRoundRect(20, 65, 280, 115, 12, TFT_WHITE);
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(text, plate);
  String color(source.colorName); color.toUpperCase();
  if (color.length() > 13) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.TFT_eSPI::drawString(color, 160, 122, 2);
  } else tft_.drawString(color, 160, 122, 4);
  tft_.setTextColor(CYAN, BG); tft_.drawString(source.material, 160, 210, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG); tft_.TFT_eSPI::drawString(source.sku, 160, 250, 2);
  tft_.setTextColor(AMBER, BG); tft_.drawString("REMOVE ALL TAGS", 160, 315, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Waiting for clear field", 160, 360, 2);
  drawFooterButton(0, 320, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=SAVED_COPY_CAPTURED");
}

void Ui::showCloneWaiting(const AceTagData &source) {
  cloneMode_ = true;
  screen_ = UiScreen::CloneWaiting;
  tft_.fillScreen(BG); drawHeader("CLONE ARMED");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(CYAN, BG);
  tft_.drawString("READY TO CLONE", 160, 125, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(String(source.material) + " " + source.colorName, 160, 205, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Present writable destination", 160, 280, 2);
  tft_.TFT_eSPI::drawString("Hold tag steady", 160, 315, 2);
  drawFooterButton(0, 320, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=CLONE_WAITING");
}

void Ui::showCloneSuccess(const AceTagData &source, const String &uid) {
  screen_ = UiScreen::CloneSuccess;
  tft_.fillScreen(BG); drawHeader("CLONE COMPLETE");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_GREEN, BG);
  tft_.drawString("CLONE COMPLETE", 160, 105, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(String(source.material) + " " + source.colorName, 160, 175, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_GREEN, BG);
  tft_.TFT_eSPI::drawString("Write     PASS", 160, 245, 2);
  tft_.TFT_eSPI::drawString("Verify    PASS", 160, 280, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Destination UID", 160, 330, 2);
  tft_.TFT_eSPI::drawString(uid, 160, 360, 2);
  drawFooterButton(0, 320, "DONE", HEADER); tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=CLONE_SUCCESS");
}

void Ui::restoreRetainedAce() {
  cloneMode_ = false;
  screen_ = UiScreen::AceResult;
  tagPresent_ = false;
  drawAceResult();
  Serial.println("[ui] Clone cancelled; retained ACE result restored");
}

void Ui::showLibrary(const std::vector<LibraryEntry> &entries, bool storageAvailable,
                     size_t totalBytes, size_t usedBytes, uint16_t invalidCount) {
  libraryEntries_ = &entries;
  storageAvailable_ = storageAvailable;
  storageTotal_ = totalBytes;
  storageUsed_ = usedBytes;
  invalidFiles_ = invalidCount;
  const int maxOffset = max(0, static_cast<int>(entries.size()) - 5);
  libraryScroll_ = min(static_cast<int>(libraryScroll_), maxOffset);
  screen_ = UiScreen::LibraryList;
  drawLibrary();
  Serial.println("[ui] Screen=LIBRARY");
}

void Ui::drawLibrary() {
  screen_ = UiScreen::LibraryList;
  tft_.fillScreen(BG);
  const String count = libraryEntries_ ? String(libraryEntries_->size()) : String("0");
  drawHeader((count + " SAVED").c_str());
  if (!storageAvailable_) {
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
    tft_.drawString("STORAGE", 128, 175, 4);
    tft_.drawString("UNAVAILABLE", 128, 205, 4);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("RFID functions", 128, 250, 2);
    tft_.TFT_eSPI::drawString("remain available", 128, 275, 2);
  } else if (!libraryEntries_ || libraryEntries_->empty()) {
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_WHITE, BG);
    tft_.drawString("No saved tags yet", 128, 180, 4);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("Scan an ACE tag", 128, 230, 2);
    tft_.TFT_eSPI::drawString("then tap SAVE", 128, 255, 2);
  } else {
    tft_.setTextDatum(TL_DATUM);
    for (uint8_t row = 0; row < 5; ++row) {
      const size_t index = libraryScroll_ + row;
      if (index >= libraryEntries_->size()) break;
      const LibraryEntry &entry = (*libraryEntries_)[index];
      const int16_t y = 64 + row * 70;
      tft_.drawRect(8, y, 240, 64, HEADER);
      const uint16_t swatch = ((entry.red & 0xF8) << 8) |
                              ((entry.green & 0xFC) << 3) | (entry.blue >> 3);
      tft_.fillRoundRect(15, y + 12, 38, 38, 6, swatch);
      tft_.drawRoundRect(15, y + 12, 38, 38, 6, TFT_WHITE);
      tft_.setTextColor(TFT_WHITE, BG);
      String listName(entry.colorName);
      if (listName.length() > 15) listName = listName.substring(0, 14) + ".";
      tft_.drawString(listName, 64, y + 8, 4);
      tft_.setTextColor(MUTED, BG);
      tft_.drawString(String(entry.material) + "  " + entry.sku, 64, y + 38, 2);
      tft_.setTextDatum(TR_DATUM);
      tft_.drawString("#" + String(entry.id), 240, y + 35, 2);
      tft_.setTextDatum(TL_DATUM);
    }
  }
  const bool hasPrevious = libraryScroll_ > 0;
  const bool hasNext = libraryEntries_ && libraryScroll_ + 5 < libraryEntries_->size();
  const uint16_t previousColor = hasPrevious ? CYAN : HEADER;
  const uint16_t nextColor = hasNext ? CYAN : HEADER;
  tft_.fillRoundRect(256, 64, 64, 169, 7, previousColor);
  tft_.drawRoundRect(256, 64, 64, 169, 7, MUTED);
  tft_.fillRoundRect(256, 239, 64, 175, 7, nextColor);
  tft_.drawRoundRect(256, 239, 64, 175, 7, MUTED);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(hasPrevious ? TFT_BLACK : MUTED, previousColor);
  tft_.drawString("PREV", 288, 148, 4);
  tft_.setTextColor(hasNext ? TFT_BLACK : MUTED, nextColor);
  tft_.drawString("NEXT", 288, 326, 4);
  tft_.setTextDatum(TL_DATUM);
  drawFooterButton(0, 156, "STORAGE", HEADER);
  drawFooterButton(160, 160, "BACK", 0x2124);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showSavedDetail(uint32_t id, const AceTagData &tag) {
  selectedLibraryId_ = id;
  savedTag_ = tag;
  screen_ = UiScreen::SavedDetail;
  drawSavedDetail();
  Serial.printf("[ui] Screen=SAVED_DETAIL id=%lu\n", static_cast<unsigned long>(id));
}

void Ui::showEmulationWaiting(const AceTagData &tag, bool fromSaved) {
  emulationFromSaved_ = fromSaved;
  screen_ = UiScreen::EmulateWaiting;
  tft_.fillScreen(BG);
  drawHeader("READ-ONLY EMULATION");
  const uint16_t plate = AceCodec::rgb565(tag);
  const uint16_t text = AceCodec::contrastText(tag);
  tft_.fillRoundRect(20, 75, 280, 110, 12, plate);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(text, plate);
  tft_.drawString(tag.colorName, 160, 112, 4);
  tft_.setTextColor(CYAN, BG);
  tft_.drawString(tag.material, 160, 220, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(fromSaved ? "Present PN532 to ACE sensor" :
                              "Remove source tag, then present to ACE",
                  160, 285, 2);
  tft_.setTextColor(AMBER, BG);
  tft_.drawString("Touch disabled during RF session", 160, 335, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showEmulationResult(bool complete, const char *detail) {
  screen_ = UiScreen::EmulateResult;
  tft_.fillScreen(BG);
  drawHeader("EMULATION RESULT");
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(complete ? TFT_GREEN : TFT_RED, BG);
  tft_.drawString(complete ? "READ COMPLETE" : "READ FAILED", 160, 165, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(detail, 160, 245, 2);
  tft_.setTextColor(MUTED, BG);
  tft_.drawString("Remove handheld from ACE sensor", 160, 315, 2);
  drawFooterButton(0, 320, "DONE", 0x2124);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawSavedDetail() {
  const AceTagData &tag = savedTag_;
  tft_.fillScreen(BG); drawHeader("SAVED TAG");
  const uint16_t plate = AceCodec::rgb565(tag);
  const uint16_t text = AceCodec::contrastText(tag);
  tft_.fillRoundRect(20, 60, 280, 105, 12, plate);
  tft_.drawRoundRect(20, 60, 280, 105, 12, TFT_WHITE);
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(text, plate);
  String color(tag.colorName); color.toUpperCase();
  if (color.length() > 13) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.TFT_eSPI::drawString(color, 160, 112, 2);
  } else tft_.drawString(color, 160, 112, 4);
  tft_.setTextColor(CYAN, BG); tft_.drawString(tag.material, 160, 185, 4);
  tft_.setTextDatum(TL_DATUM);
  auto row = [this](int16_t y, const char *label, const String &value) {
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG); tft_.TFT_eSPI::drawString(label, 20, y, 2);
    tft_.setTextDatum(TR_DATUM); tft_.setTextColor(TFT_WHITE, BG);
    tft_.TFT_eSPI::drawString(value, 300, y, 2); tft_.setTextDatum(TL_DATUM);
  };
  row(210, "SKU", tag.sku);
  row(238, "Nozzle", String(tag.nozzleMin) + " - " + tag.nozzleMax + " C");
  row(266, "Bed", String(tag.bedMin) + " - " + tag.bedMax + " C");
  row(294, "Diameter", String(tag.diameterHundredthsMm / 100.0f, 2) + " mm");
  row(322, "Length", String(tag.lengthMeters) + " m");
  row(350, "Source UID", tag.uidText);
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(MUTED, BG);
  auto savedAction = [this](int16_t x, int16_t width, const char *label,
                            uint16_t fill, uint16_t textColor) {
    tft_.fillRect(x, 376, width, 52, fill);
    tft_.drawRect(x, 376, width, 52, MUTED);
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(textColor, fill);
    tft_.drawString(label, x + width / 2, 402, 4);
  };
  savedAction(0, 76, "RAW", HEADER, TFT_WHITE);
  savedAction(80, 76, "EMU", AMBER, TFT_BLACK);
  savedAction(160, 76, "WRITE", CYAN, TFT_BLACK);
  savedAction(240, 80, "DELETE", 0x7800, TFT_WHITE);
  drawFooterButton(0, 320, "BACK", 0x2124);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showSaveDuplicate() {
  screen_ = UiScreen::SaveDuplicate;
  tft_.fillScreen(BG); drawHeader("ALREADY SAVED");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(AMBER, BG);
  tft_.drawString("TAG ALREADY SAVED", 160, 150, 4);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString(String(cachedTag_.material) + " " + cachedTag_.colorName, 160, 220, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(MUTED, BG);
  tft_.TFT_eSPI::drawString("Save another copy?", 160, 290, 2);
  drawFooterButton(0, 156, "SAVE COPY", HEADER);
  drawFooterButton(160, 160, "CANCEL", 0x2124);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::showSaveResult(bool success, uint32_t id, const char *message) {
  screen_ = UiScreen::SaveResult;
  tft_.fillScreen(BG); drawHeader(success ? "SAVED" : "SAVE FAILED");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(success ? TFT_GREEN : TFT_RED, BG);
  tft_.drawString(success ? "SAVED" : "SAVE FAILED", 160, 145, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG); tft_.TFT_eSPI::drawString(message, 160, 220, 2);
  if (success) {
    tft_.setTextColor(CYAN, BG);
    tft_.drawString(String(cachedTag_.material) + " " + cachedTag_.colorName, 160, 270, 4);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("Library ID " + String(id), 160, 325, 2);
  }
  drawFooterButton(0, 320, "DONE", HEADER); tft_.setTextDatum(TL_DATUM);
}

void Ui::showDeleteResult(bool success) {
  if (success) return;
  screen_ = UiScreen::SaveResult;
  tft_.fillScreen(BG); drawHeader("DELETE FAILED");
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(TFT_RED, BG);
  tft_.drawString("DELETE FAILED", 160, 160, 4);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.TFT_eSPI::drawString("Saved tag was not removed", 160, 235, 2);
  if (storageAvailable_) drawFooterButton(0, 320, "BACK", HEADER);
  else {
    drawFooterButton(0, 156, "BACK", HEADER);
    drawFooterButton(160, 160, "FORMAT", TFT_RED);
  }
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawStorageInfo() {
  tft_.fillScreen(BG); drawHeader("INTERNAL STORAGE");
  tft_.setTextDatum(MC_DATUM);
  if (!storageAvailable_) {
    tft_.setTextColor(TFT_RED, BG); tft_.drawString("UNAVAILABLE", 160, 175, 4);
  } else {
    tft_.setTextColor(CYAN, BG); tft_.drawString("LITTLEFS", 160, 115, 4);
    tft_.setTextColor(TFT_WHITE, BG);
    tft_.drawString("Total: " + String(storageTotal_ / 1024) + " KiB", 160, 180, 4);
    tft_.drawString("Used: " + String(storageUsed_ / 1024) + " KiB", 160, 225, 4);
    tft_.drawString("Free: " + String((storageTotal_ - storageUsed_) / 1024) + " KiB", 160, 270, 4);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(MUTED, BG);
    tft_.TFT_eSPI::drawString("Saved tags: " + String(libraryEntries_ ? libraryEntries_->size() : 0), 160, 325, 2);
    tft_.TFT_eSPI::drawString("Invalid skipped: " + String(invalidFiles_), 160, 355, 2);
  }
  drawFooterButton(0, 320, "BACK", HEADER); tft_.setTextDatum(TL_DATUM);
}

void Ui::showSetup(const char *volumeLabel, uint16_t screenTimeoutSeconds) {
  screen_ = UiScreen::Setup;
  tft_.fillScreen(BG); drawHeader("SETUP");
  auto settingButton = [this](int16_t y, const char *label, const String &value,
                              uint16_t fill, uint16_t labelColor) {
    tft_.fillRoundRect(12, y, 296, 58, 8, fill);
    tft_.drawRoundRect(12, y, 296, 58, 8, MUTED);
    // Use the same large classic typeface role as the header. Two centered
    // lines prevent long setting names and their values from overlapping.
    tft_.setTextDatum(MC_DATUM); tft_.setTextColor(labelColor, fill);
    tft_.drawString(label, 160, y + 18, 4);
    tft_.setTextColor(CYAN, fill);
    tft_.drawString(value, 160, y + 43, 4);
  };
  settingButton(60, "Touch calibration", "START", HEADER, AMBER);
  settingButton(126, "Beep volume", volumeLabel, HEADER, AMBER);
  const String screenValue = screenTimeoutSeconds == 0
                                 ? String("ALWAYS ON")
                                 : String(screenTimeoutSeconds) + " SEC";
  settingButton(192, "Screen sleep", screenValue, HEADER, AMBER);
  settingButton(258, "Erase library", "ALL SAVED TAGS", 0x7800, TFT_WHITE);
  tft_.setTextDatum(MC_DATUM); tft_.setTextColor(MUTED, BG);
  tft_.setTextFont(2); tft_.setTextSize(1);
  tft_.TFT_eSPI::drawString("PN532 firmware " + String(readerStatus_.firmwareMajor) + "." +
                           String(readerStatus_.firmwareMinor), 160, 340, 2);
  tft_.setTextColor(TFT_WHITE, BG);
  tft_.drawString("Tap a setting to change", 160, 382, 4);
  drawFooterButton(0, 320, "HOME", 0x2124);
  tft_.setTextDatum(TL_DATUM);
  Serial.println("[ui] Screen=SETUP");
}

void Ui::calibrate() {
  TS_Point samples[4];
  constexpr int16_t xs[4] = {28, 292, 292, 28};
  constexpr int16_t ys[4] = {70, 70, 452, 452};
  for (uint8_t step = 0; step < 4; ++step) {
    tft_.fillScreen(BG);
    drawHeader("CALIBRATE");
    tft_.setTextDatum(MC_DATUM);
    tft_.setTextFont(2); tft_.setTextSize(1);
    tft_.setTextColor(TFT_WHITE, BG);
    tft_.TFT_eSPI::drawString("Tap and release target " + String(step + 1) + " of 4",
                             160, 235, 2);
    tft_.drawCircle(xs[step], ys[step], 16, AMBER);
    tft_.drawFastHLine(xs[step] - 22, ys[step], 45, AMBER);
    tft_.drawFastVLine(xs[step], ys[step] - 22, 45, AMBER);
    while (touch_.touched()) delay(10);
    while (!touch_.touched()) delay(10);
    samples[step] = touch_.getPoint();
    while (touch_.touched()) delay(10);
  }

  const int left = (samples[0].x + samples[3].x) / 2;
  const int right = (samples[1].x + samples[2].x) / 2;
  const int top = (samples[0].y + samples[1].y) / 2;
  const int bottom = (samples[2].y + samples[3].y) / 2;
  if (abs(right - left) < 300 || abs(bottom - top) < 300) {
    Serial.println("[touch] Calibration rejected; retrying");
    calibrate();
    return;
  }

  constexpr float targetLeft = 28.0f, targetRight = 292.0f;
  constexpr float targetTop = 70.0f, targetBottom = 452.0f;
  const float xScale = (right - left) / (targetRight - targetLeft);
  const float yScale = (bottom - top) / (targetBottom - targetTop);
  minX_ = lroundf(left - xScale * targetLeft);
  maxX_ = lroundf(left + xScale * ((Hardware::SCREEN_WIDTH - 1) - targetLeft));
  minY_ = lroundf(top - yScale * targetTop);
  maxY_ = lroundf(top + yScale * ((Hardware::SCREEN_HEIGHT - 1) - targetTop));

  preferences_.begin("ace-handheld", false);
  preferences_.putInt("touchMinX", minX_);
  preferences_.putInt("touchMaxX", maxX_);
  preferences_.putInt("touchMinY", minY_);
  preferences_.putInt("touchMaxY", maxY_);
  preferences_.putBool("touchCal", true);
  preferences_.end();
  Serial.printf("[touch] Calibration saved x=%d..%d y=%d..%d\n", minX_, maxX_, minY_, maxY_);
}
