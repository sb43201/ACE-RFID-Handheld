#include "Pn532TargetEmulator.h"

namespace {
constexpr uint8_t SPI_DATA_WRITE = 0x01;
constexpr uint8_t SPI_STATUS_READ = 0x02;
constexpr uint8_t SPI_DATA_READ = 0x03;
constexpr uint8_t HOST_TO_PN532 = 0xD4;
constexpr uint8_t PN532_TO_HOST = 0xD5;
constexpr uint8_t TG_INIT_AS_TARGET = 0x8C;
constexpr uint8_t TG_GET_INITIATOR_COMMAND = 0x88;
constexpr uint8_t TG_RESPONSE_TO_INITIATOR = 0x90;
constexpr uint8_t PAGE_COUNT = 45;
}

Pn532TargetEmulator::Pn532TargetEmulator(uint8_t chipSelect, SPIClass &spi)
    : chipSelect_(chipSelect), spi_(spi) {}

void Pn532TargetEmulator::select() {
  spi_.beginTransaction(settings_);
  digitalWrite(chipSelect_, LOW);
}

void Pn532TargetEmulator::deselect() {
  digitalWrite(chipSelect_, HIGH);
  spi_.endTransaction();
}

bool Pn532TargetEmulator::ready() {
  select();
  spi_.transfer(SPI_STATUS_READ);
  const bool value = spi_.transfer(0x00) == 0x01;
  deselect();
  return value;
}

bool Pn532TargetEmulator::waitReady(uint32_t timeoutMs, bool allowCancel) {
  const uint32_t started = millis();
  do {
    // Check the PN532 first. In particular, never insert a touchscreen SPI
    // transaction ahead of a command ACK that is already waiting.
    if (ready()) return true;
    const uint32_t now = millis();
    if (allowCancel && cancel_ && now - lastCancelPollMs_ >= 20) {
      lastCancelPollMs_ = now;
      if (cancel_()) {
        cancelled_ = true;
        return false;
      }
    }
    delay(2);
  } while (millis() - started < timeoutMs);
  return false;
}

void Pn532TargetEmulator::writeFrame(const uint8_t *command, size_t length) {
  const uint8_t frameLength = static_cast<uint8_t>(length + 1);
  uint8_t checksum = HOST_TO_PN532;
  select();
  spi_.transfer(SPI_DATA_WRITE);
  spi_.transfer(0x00); spi_.transfer(0x00); spi_.transfer(0xFF);
  spi_.transfer(frameLength);
  spi_.transfer(static_cast<uint8_t>(~frameLength + 1));
  spi_.transfer(HOST_TO_PN532);
  for (size_t i = 0; i < length; ++i) {
    spi_.transfer(command[i]);
    checksum = static_cast<uint8_t>(checksum + command[i]);
  }
  spi_.transfer(static_cast<uint8_t>(~checksum + 1));
  spi_.transfer(0x00);
  deselect();
}

bool Pn532TargetEmulator::readAck(uint32_t timeoutMs) {
  static const uint8_t expected[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
  // The touchscreen shares VSPI. Do not poll it inside the short, critical
  // PN532 ACK window; cancellation remains available while awaiting response.
  if (!waitReady(timeoutMs, false)) return false;
  uint8_t actual[sizeof(expected)];
  select();
  spi_.transfer(SPI_DATA_READ);
  for (uint8_t &value : actual) value = spi_.transfer(0x00);
  deselect();
  return memcmp(actual, expected, sizeof(expected)) == 0;
}

bool Pn532TargetEmulator::readResponse(uint8_t commandCode, uint8_t *data,
                                       size_t capacity, size_t &length,
                                       uint32_t timeoutMs) {
  length = 0;
  if (!waitReady(timeoutMs, true)) return false;
  select();
  spi_.transfer(SPI_DATA_READ);
  const uint8_t p0 = spi_.transfer(0), p1 = spi_.transfer(0), p2 = spi_.transfer(0);
  const uint8_t frameLength = spi_.transfer(0), lcs = spi_.transfer(0);
  if (p0 || p1 || p2 != 0xFF || static_cast<uint8_t>(frameLength + lcs) != 0) {
    deselect();
    return false;
  }
  uint8_t sum = spi_.transfer(0);
  const uint8_t tfi = sum;
  const uint8_t responseCode = spi_.transfer(0);
  sum = static_cast<uint8_t>(sum + responseCode);
  if (frameLength < 2) { deselect(); return false; }
  const size_t payloadLength = frameLength - 2;
  for (size_t i = 0; i < payloadLength; ++i) {
    const uint8_t value = spi_.transfer(0);
    sum = static_cast<uint8_t>(sum + value);
    if (i < capacity) data[i] = value;
  }
  const uint8_t dcs = spi_.transfer(0);
  spi_.transfer(0);
  deselect();
  if (tfi != PN532_TO_HOST || responseCode != static_cast<uint8_t>(commandCode + 1) ||
      static_cast<uint8_t>(sum + dcs) != 0 || payloadLength > capacity)
    return false;
  length = payloadLength;
  return true;
}

bool Pn532TargetEmulator::command(uint8_t code, const uint8_t *parameters,
                                  size_t parameterLength, uint8_t *response,
                                  size_t capacity, size_t &responseLength,
                                  uint32_t responseTimeoutMs) {
  uint8_t frame[64];
  if (parameterLength + 1 > sizeof(frame)) return false;
  frame[0] = code;
  if (parameterLength) memcpy(frame + 1, parameters, parameterLength);
  writeFrame(frame, parameterLength + 1);
  if (!readAck(1000)) return false;
  return readResponse(code, response, capacity, responseLength, responseTimeoutMs);
}

void Pn532TargetEmulator::copyPage(const AceTagData &tag, uint8_t page,
                                   uint8_t output[4]) const {
  memset(output, 0, 4);
  if (page >= 4 && page <= 31) {
    memcpy(output, tag.pages[page], 4);
    return;
  }
  if (tag.uidLength == 7) {
    if (page == 0) {
      output[0] = tag.uid[0]; output[1] = tag.uid[1]; output[2] = tag.uid[2];
      output[3] = 0x88 ^ tag.uid[0] ^ tag.uid[1] ^ tag.uid[2];
    } else if (page == 1) {
      memcpy(output, tag.uid + 3, 4);
    } else if (page == 2) {
      output[0] = tag.uid[3] ^ tag.uid[4] ^ tag.uid[5] ^ tag.uid[6];
      output[1] = 0x48;
    }
  }
  if (page == 3) {
    const uint8_t cc[] = {0xE1, 0x10, 0x12, 0x00};
    memcpy(output, cc, 4);
  } else if (page == 40) {
    const uint8_t value[] = {0x00, 0x00, 0x00, 0xBD};
    memcpy(output, value, 4);
  } else if (page == 41) {
    const uint8_t value[] = {0x04, 0x00, 0x00, 0x04};
    memcpy(output, value, 4);
  } else if (page == 42) {
    const uint8_t value[] = {0x47, 0x00, 0x00, 0x00};
    memcpy(output, value, 4);
  }
}

bool Pn532TargetEmulator::respond(const uint8_t *data, size_t length) {
  uint8_t response[8];
  size_t responseLength = 0;
  if (!command(TG_RESPONSE_TO_INITIATOR, data, length, response,
               sizeof(response), responseLength, 1000))
    return false;
  return responseLength == 0 || response[0] == 0x00;
}

bool Pn532TargetEmulator::handleInitiatorCommand(const AceTagData &tag,
                                                  const uint8_t *data,
                                                  size_t length,
                                                  bool &completed,
                                                  ProgressCallback progress) {
  if (length == 2 && data[0] == 0x30 && data[1] < PAGE_COUNT) {
    const uint8_t startPage = data[1];
    uint8_t reply[16];
    for (uint8_t offset = 0; offset < 4; ++offset)
      copyPage(tag, (startPage + offset) % PAGE_COUNT, reply + offset * 4);
    const uint32_t receivedAt = millis();
    const bool sent = respond(reply, sizeof(reply));
    const uint32_t sentAt = millis();
    Serial.printf("[emu RX %06lu ms] 30 %02X\n", receivedAt, startPage);
    Serial.printf("[emu TX %06lu ms] ", sentAt);
    printHex(reply, sizeof(reply));
    Serial.println();
    // The callback draws only one small preallocated progress-bar segment.
    // It runs after the RF reply has completed, never before the response.
    if (sent && progress) progress(startPage);
    if (sent && startPage == 0x24) completed = true;
    return sent;
  }
  Serial.printf("[emu RX %06lu ms] ", millis());
  printHex(data, length);
  Serial.println();
  if (length && (data[0] == 0xA2 || data[0] == 0xA0))
    Serial.println("[emu] Write refused (read-only)");
  else
    Serial.println("[emu] Unsupported command; no response");
  return true;
}

EmulationResult Pn532TargetEmulator::run(const AceTagData &tag,
                                         uint32_t activationTimeoutMs,
                                         ProgressCallback progress,
                                         CancelCallback cancel) {
  static const uint8_t targetParameters[] = {
    0x00,
    0x04, 0x00, 0x12, 0x34, 0x56, 0x00,
    0x01, 0xFE, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF,
    0x01, 0xFE, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x00, 0x00,
    0x00, 0x00
  };
  uint8_t response[32];
  size_t responseLength = 0;
  cancel_ = cancel;
  cancelled_ = false;
  lastCancelPollMs_ = millis() - 20;
  Serial.printf("[emu] Waiting for ACE: %s %s\n", tag.material, tag.colorName);
  const uint32_t activationStarted = millis();
  if (!command(TG_INIT_AS_TARGET, targetParameters, sizeof(targetParameters),
               response, sizeof(response), responseLength, activationTimeoutMs)) {
    if (cancelled_) return EmulationResult::Cancelled;
    // ACK/transport failures return in roughly one second. Only report a
    // no-reader timeout when the full activation window actually elapsed.
    return millis() - activationStarted + 100 >= activationTimeoutMs
               ? EmulationResult::Timeout
               : EmulationResult::TransportError;
  }

  bool completed = false;
  if (responseLength > 1 &&
      !handleInitiatorCommand(tag, response + 1, responseLength - 1,
                              completed, progress))
    return EmulationResult::TransportError;

  while (true) {
    responseLength = 0;
    if (!command(TG_GET_INITIATOR_COMMAND, nullptr, 0, response,
                 sizeof(response), responseLength, 1000)) {
      if (cancelled_) return EmulationResult::Cancelled;
      return EmulationResult::TransportError;
    }
    if (!responseLength || response[0] != 0x00)
      return completed ? EmulationResult::Complete : EmulationResult::Interrupted;
    if (!handleInitiatorCommand(tag, response + 1, responseLength - 1,
                                completed, progress))
      return EmulationResult::TransportError;
  }
}

void Pn532TargetEmulator::printHex(const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    if (i) Serial.print(' ');
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
  }
}
