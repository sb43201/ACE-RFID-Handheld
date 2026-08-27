#pragma once

#include <Arduino.h>
#include <Preferences.h>

class MelodyPlayer {
 public:
  enum class Cue : uint8_t { Startup, TagFound, Error };
  enum class Volume : uint8_t { High, Mid, Low, Mute };
  struct Note {
    uint16_t frequency;
    uint16_t durationMs;
    uint16_t gapMs;
  };

  void begin();
  void play(Cue cue);
  void update();
  bool playing() const { return notes_ != nullptr; }
  void setVolume(Volume volume);
  Volume volume() const { return volume_; }
  const char *volumeLabel() const;

 private:
  void startCurrentNote();
  void stopOutput();

  static constexpr uint8_t PWM_CHANNEL = 7;
  static constexpr uint8_t PWM_RESOLUTION = 8;
  // ledcWriteTone defaults to 50% (128/255), which is unnecessarily loud
  // through the E32R35T onboard amplifier. This is roughly 9% duty.
  uint8_t noteDuty() const;
  Preferences preferences_;
  Volume volume_ = Volume::Mid;
  const Note *notes_ = nullptr;
  uint8_t noteCount_ = 0;
  uint8_t noteIndex_ = 0;
  uint32_t phaseEndsMs_ = 0;
  bool gapPhase_ = false;
};
