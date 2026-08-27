#pragma once

#include <Arduino.h>

namespace Hardware {
constexpr uint8_t SCREEN_ROTATION = 0;
constexpr int16_t SCREEN_WIDTH = 320;
constexpr int16_t SCREEN_HEIGHT = 480;

constexpr uint8_t TFT_BACKLIGHT = 27;
constexpr uint8_t TOUCH_CS_PIN = 33;
constexpr uint8_t TOUCH_IRQ_PIN = 36;

// Confirmed by the vendor E32R35T Audio_WAV demo. GPIO26 feeds the
// onboard analog amplifier; GPIO4 enables it when driven LOW.
constexpr uint8_t AUDIO_DAC = 26;
constexpr uint8_t AUDIO_ENABLE = 4;
constexpr uint8_t BATTERY_ADC = 34;

// PN532 uses the exposed VSPI signals. The onboard SD socket shares this bus,
// but has a separate chip select (SD=GPIO5, PN532=GPIO21).
constexpr uint8_t PN532_SCK = 18;
constexpr uint8_t PN532_MISO = 19;
constexpr uint8_t PN532_MOSI = 23;
constexpr uint8_t PN532_SS = 21;
}  // namespace Hardware
