#pragma once

#include <Arduino.h>
#include <esp_adc_cal.h>

class BatteryMonitor {
 public:
  void begin();
  bool update(bool force = false);
  uint16_t millivolts() const { return millivolts_; }
  uint8_t percent() const { return percent_; }

 private:
  esp_adc_cal_characteristics_t adcCharacteristics_{};
  uint32_t nextSampleMs_ = 0;
  uint16_t millivolts_ = 0;
  uint8_t percent_ = 0;
};
