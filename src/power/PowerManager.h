#pragma once

#include <Arduino.h>
#include <esp_sleep.h>
#include <Preferences.h>

class PowerManager {
 public:
  void begin();
  void handleTouch(bool down);
  void wakeDisplayForRfid();
  void update(bool operationBusy);
  void setScreenTimeoutSeconds(uint16_t seconds);
  uint16_t screenTimeoutSeconds() const { return screenTimeoutSeconds_; }
  bool wokeByTouch() const { return wokeByTouch_; }
  bool controlsAllowed() const {
    return screenAwake_ && !wakeTouchInProgress_ && !wakeReleaseGateActive_;
  }

 private:
  void setBacklight(uint8_t duty);
  void wakeDisplay();
  void enterDeepSleep();

  static constexpr uint32_t DEEP_SLEEP_TIMEOUT_MS = 180000;
  static constexpr uint8_t BACKLIGHT_ACTIVE = 192;
  static constexpr uint8_t BACKLIGHT_DIM = 64;
  static constexpr uint8_t BACKLIGHT_PWM_CHANNEL = 0;
  static constexpr uint32_t BACKLIGHT_PWM_HZ = 5000;

  uint32_t lastUserTouchMs_ = 0;
  bool touchWasDown_ = false;
  bool wakeTouchInProgress_ = false;
  bool wakeReleaseGateActive_ = false;
  bool screenAwake_ = true;
  bool dimmed_ = false;
  bool wokeByTouch_ = false;
  uint16_t screenTimeoutSeconds_ = 60;
  uint32_t dimTimeoutMs_ = 30000;
  uint32_t screenTimeoutMs_ = 60000;
  Preferences preferences_;
};
