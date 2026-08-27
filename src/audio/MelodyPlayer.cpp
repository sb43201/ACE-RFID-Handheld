#include "MelodyPlayer.h"

#include "../config/Hardware.h"

namespace {
constexpr MelodyPlayer::Note STARTUP_NOTES[] = {
    {523, 80, 25}, {659, 80, 25}, {784, 145, 0}};   // C5 E5 G5
constexpr MelodyPlayer::Note TAG_NOTES[] = {
    {659, 70, 18}, {784, 70, 18}, {1047, 135, 0}};  // E5 G5 C6
constexpr MelodyPlayer::Note ERROR_NOTES[] = {
    {659, 100, 25}, {494, 120, 25}, {330, 210, 0}}; // E5 B4 E4
}  // namespace

void MelodyPlayer::begin() {
  preferences_.begin("ace-handheld", true);
  volume_ = static_cast<Volume>(min<uint8_t>(preferences_.getUChar("beepVol", 1), 3));
  preferences_.end();
  pinMode(Hardware::AUDIO_ENABLE, OUTPUT);
  digitalWrite(Hardware::AUDIO_ENABLE, HIGH);
  ledcSetup(PWM_CHANNEL, 1000, PWM_RESOLUTION);
  ledcAttachPin(Hardware::AUDIO_DAC, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);
  Serial.printf("[audio] Onboard speaker ready signal=GPIO26 enable=GPIO4(active-low) volume=%s\n",
                volumeLabel());
}

void MelodyPlayer::play(Cue cue) {
  if (volume_ == Volume::Mute) return;
  switch (cue) {
    case Cue::Startup:
      notes_ = STARTUP_NOTES;
      noteCount_ = sizeof(STARTUP_NOTES) / sizeof(STARTUP_NOTES[0]);
      break;
    case Cue::TagFound:
      notes_ = TAG_NOTES;
      noteCount_ = sizeof(TAG_NOTES) / sizeof(TAG_NOTES[0]);
      break;
    case Cue::Error:
      notes_ = ERROR_NOTES;
      noteCount_ = sizeof(ERROR_NOTES) / sizeof(ERROR_NOTES[0]);
      break;
  }
  noteIndex_ = 0;
  gapPhase_ = false;
  digitalWrite(Hardware::AUDIO_ENABLE, LOW);
  startCurrentNote();
}

void MelodyPlayer::update() {
  if (!notes_ || static_cast<int32_t>(millis() - phaseEndsMs_) < 0) return;
  const Note &note = notes_[noteIndex_];
  if (!gapPhase_ && note.gapMs) {
    ledcWriteTone(PWM_CHANNEL, 0);
    gapPhase_ = true;
    phaseEndsMs_ = millis() + note.gapMs;
    return;
  }
  ++noteIndex_;
  gapPhase_ = false;
  if (noteIndex_ >= noteCount_) {
    stopOutput();
    return;
  }
  startCurrentNote();
}

void MelodyPlayer::startCurrentNote() {
  const Note &note = notes_[noteIndex_];
  ledcWriteTone(PWM_CHANNEL, note.frequency);
  ledcWrite(PWM_CHANNEL, noteDuty());
  phaseEndsMs_ = millis() + note.durationMs;
}

void MelodyPlayer::setVolume(Volume volume) {
  volume_ = volume;
  preferences_.begin("ace-handheld", false);
  preferences_.putUChar("beepVol", static_cast<uint8_t>(volume_));
  preferences_.end();
  if (volume_ == Volume::Mute) stopOutput();
  Serial.printf("[audio] Volume=%s\n", volumeLabel());
}

const char *MelodyPlayer::volumeLabel() const {
  switch (volume_) {
    case Volume::High: return "HIGH";
    case Volume::Mid: return "MID";
    case Volume::Low: return "LOW";
    default: return "MUTE";
  }
}

uint8_t MelodyPlayer::noteDuty() const {
  switch (volume_) {
    case Volume::High: return 24;
    case Volume::Mid: return 12;
    case Volume::Low: return 5;
    default: return 0;
  }
}

void MelodyPlayer::stopOutput() {
  ledcWriteTone(PWM_CHANNEL, 0);
  digitalWrite(Hardware::AUDIO_ENABLE, HIGH);
  notes_ = nullptr;
  noteCount_ = 0;
}
