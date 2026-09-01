#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "AceTag.h"

enum class EmulationResult : uint8_t {
  Complete,
  Cancelled,
  Interrupted,
  Timeout,
  TransportError
};

class Pn532TargetEmulator {
 public:
  using ProgressCallback = void (*)(uint8_t startPage);
  using CancelCallback = bool (*)();
  Pn532TargetEmulator(uint8_t chipSelect, SPIClass &spi);
  EmulationResult run(const AceTagData &tag, uint32_t activationTimeoutMs = 60000,
                      ProgressCallback progress = nullptr,
                      CancelCallback cancel = nullptr);

 private:
  bool ready();
  bool waitReady(uint32_t timeoutMs, bool allowCancel = false);
  void writeFrame(const uint8_t *command, size_t length);
  bool readAck(uint32_t timeoutMs);
  bool readResponse(uint8_t command, uint8_t *data, size_t capacity,
                    size_t &length, uint32_t timeoutMs);
  bool command(uint8_t code, const uint8_t *parameters, size_t parameterLength,
               uint8_t *response, size_t capacity, size_t &responseLength,
               uint32_t responseTimeoutMs);
  bool respond(const uint8_t *data, size_t length);
  bool handleInitiatorCommand(const AceTagData &tag, const uint8_t *data,
                              size_t length, bool &completed,
                              ProgressCallback progress);
  void copyPage(const AceTagData &tag, uint8_t page, uint8_t output[4]) const;
  static void printHex(const uint8_t *data, size_t length);
  void select();
  void deselect();

  uint8_t chipSelect_;
  SPIClass &spi_;
  SPISettings settings_{1000000, LSBFIRST, SPI_MODE0};
  CancelCallback cancel_ = nullptr;
  bool cancelled_ = false;
  uint32_t lastCancelPollMs_ = 0;
};
