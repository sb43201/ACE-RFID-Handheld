#include "PowerManager.h"

#include <driver/rtc_io.h>

#include "../config/Hardware.h"

void PowerManager::begin() {
  preferences_.begin("ace-handheld", true);
  const uint16_t savedTimeout = preferences_.getUShort("screenSec", 60);
  preferences_.end();
  screenTimeoutSeconds_ = (savedTimeout == 0 || savedTimeout == 30 ||
                           savedTimeout == 60 || savedTimeout == 120) ? savedTimeout : 60;
  screenTimeoutMs_ = static_cast<uint32_t>(screenTimeoutSeconds_) * 1000UL;
  dimTimeoutMs_ = screenTimeoutMs_ / 2;
  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  wokeByTouch_ = cause == ESP_SLEEP_WAKEUP_EXT0;
  if (wokeByTouch_) rtc_gpio_deinit(static_cast<gpio_num_t>(Hardware::TOUCH_IRQ_PIN));
  pinMode(Hardware::TOUCH_IRQ_PIN, INPUT);

  ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_HZ, 8);
  ledcAttachPin(Hardware::TFT_BACKLIGHT, BACKLIGHT_PWM_CHANNEL);
  setBacklight(BACKLIGHT_ACTIVE);
  lastUserTouchMs_ = millis();

  if (wokeByTouch_) {
    wakeReleaseGateActive_ = true;
    wakeTouchInProgress_ = true;
    Serial.println("[power] Wake: touchscreen EXT0; touch inhibited until release");
  } else {
    Serial.println("[power] Wake: power-on/reset");
  }
  if (screenTimeoutSeconds_)
    Serial.printf("[power] Backlight dim=%us off=%us deep-sleep=180s\n",
                  screenTimeoutSeconds_ / 2, screenTimeoutSeconds_);
  else Serial.println("[power] Screen always on; dim/off/deep-sleep disabled");
}

void PowerManager::handleTouch(bool down) {
  const uint32_t now = millis();
  if (wakeReleaseGateActive_) {
    if (!down && digitalRead(Hardware::TOUCH_IRQ_PIN) == HIGH) {
      wakeReleaseGateActive_ = false;
      wakeTouchInProgress_ = false;
      touchWasDown_ = false;
      lastUserTouchMs_ = now;
      Serial.println("[power] Wake touch released; input armed");
    }
    return;
  }

  if (down && !touchWasDown_) {
    touchWasDown_ = true;
    lastUserTouchMs_ = now;
    if (!screenAwake_ || dimmed_) {
      wakeTouchInProgress_ = true;
      wakeDisplay();
      Serial.println("[power] Wake touch consumed; release before control input");
    }
    return;
  }

  if (down) {
    lastUserTouchMs_ = now;
    return;
  }

  if (touchWasDown_) {
    touchWasDown_ = false;
    if (wakeTouchInProgress_) {
      wakeTouchInProgress_ = false;
      Serial.println("[power] Wake touch released; input armed");
    }
  }
}

void PowerManager::wakeDisplayForRfid() {
  if (!screenAwake_ || dimmed_) {
    wakeDisplay();
    Serial.println("[power] Display awakened by RFID activity");
  }
}

void PowerManager::update(bool operationBusy) {
  // Busy operations inhibit deep sleep but are not synthetic user activity.
  // This lets every UI screen dim/off normally while still guaranteeing that
  // an RFID write or verification cannot be interrupted by deep sleep.
  const uint32_t idleMs = millis() - lastUserTouchMs_;
  if (dimTimeoutMs_ && screenAwake_ && !dimmed_ && idleMs >= dimTimeoutMs_) {
    dimmed_ = true;
    setBacklight(BACKLIGHT_DIM);
    Serial.printf("[power] Backlight dimmed after %u seconds\n", screenTimeoutSeconds_ / 2);
  }
  if (screenTimeoutMs_ && screenAwake_ && idleMs >= screenTimeoutMs_) {
    screenAwake_ = false;
    dimmed_ = false;
    setBacklight(0);
    Serial.printf("[power] Display off after %u seconds; RFID scan continues\n",
                  screenTimeoutSeconds_);
  }
  if (screenTimeoutSeconds_ && idleMs >= DEEP_SLEEP_TIMEOUT_MS &&
      !operationBusy && !touchWasDown_ &&
      digitalRead(Hardware::TOUCH_IRQ_PIN) == HIGH) {
    enterDeepSleep();
  }
}

void PowerManager::setScreenTimeoutSeconds(uint16_t seconds) {
  if (seconds != 0 && seconds != 30 && seconds != 60 && seconds != 120) return;
  screenTimeoutSeconds_ = seconds;
  screenTimeoutMs_ = static_cast<uint32_t>(seconds) * 1000UL;
  dimTimeoutMs_ = screenTimeoutMs_ / 2;
  preferences_.begin("ace-handheld", false);
  preferences_.putUShort("screenSec", seconds);
  preferences_.end();
  lastUserTouchMs_ = millis();
  if (!screenAwake_ || dimmed_) wakeDisplay();
  Serial.printf("[power] Screen mode=%s\n", seconds ? (String(seconds) + " SEC").c_str() : "ALWAYS ON");
}

void PowerManager::setBacklight(uint8_t duty) {
  ledcWrite(BACKLIGHT_PWM_CHANNEL, duty);
}

void PowerManager::wakeDisplay() {
  screenAwake_ = true;
  dimmed_ = false;
  lastUserTouchMs_ = millis();
  setBacklight(BACKLIGHT_ACTIVE);
}

void PowerManager::enterDeepSleep() {
  setBacklight(0);
  digitalWrite(Hardware::AUDIO_ENABLE, HIGH);
  const gpio_num_t wakePin = static_cast<gpio_num_t>(Hardware::TOUCH_IRQ_PIN);
  if (!esp_sleep_is_valid_wakeup_gpio(wakePin)) {
    Serial.println("[power] ERROR: touch IRQ is not a valid EXT0 wake pin");
    lastUserTouchMs_ = millis();
    return;
  }
  const esp_err_t result = esp_sleep_enable_ext0_wakeup(wakePin, 0);
  if (result != ESP_OK) {
    Serial.printf("[power] ERROR: EXT0 configuration failed: %d\n", result);
    lastUserTouchMs_ = millis();
    return;
  }
  rtc_gpio_pullup_dis(wakePin);
  rtc_gpio_pulldown_dis(wakePin);
  Serial.flush();
  esp_deep_sleep_start();
}
