#include "BatteryMonitor.h"

#include "../config/Hardware.h"

void BatteryMonitor::begin() {
  analogReadResolution(12);
  analogSetPinAttenuation(Hardware::BATTERY_ADC, ADC_11db);
  // IDF names the current equivalent attenuation ADC_ATTEN_DB_12; it is the
  // same range previously exposed as the vendor demo's ADC_ATTEN_DB_11.
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12,
                           1100, &adcCharacteristics_);
  update(true);
  Serial.printf("[battery] GPIO34 divider=2:1 voltage=%umV level=%u%%\n",
                millivolts_, percent_);
}

bool BatteryMonitor::update(bool force) {
  const uint32_t now = millis();
  if (!force && static_cast<int32_t>(now - nextSampleMs_) < 0) return false;
  nextSampleMs_ = now + 10000;
  uint32_t rawTotal = 0;
  for (uint8_t sample = 0; sample < 16; ++sample) rawTotal += analogRead(Hardware::BATTERY_ADC);
  const uint32_t rawAverage = rawTotal / 16;
  millivolts_ = esp_adc_cal_raw_to_voltage(rawAverage, &adcCharacteristics_) * 2;
  // Vendor E32R35T demo maps its usable battery indication from 2.5 to 4.2 V.
  percent_ = millivolts_ <= 2500 ? 0 : millivolts_ >= 4200 ? 100 :
             static_cast<uint8_t>((millivolts_ - 2500) / 17);
  return true;
}
