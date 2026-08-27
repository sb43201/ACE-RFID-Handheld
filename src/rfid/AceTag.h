#pragma once

#include <Arduino.h>

struct AceTagData {
  uint8_t uid[10]{};
  uint8_t uidLength = 0;
  uint8_t pages[32][4]{};
  bool readOk = false;
  bool aceValid = false;
  int8_t failedPage = -1;
  char name[40]{};
  char material[17]{};
  char colorName[32]{};
  char colorHex[9]{};
  char sku[21]{};
  char brand[21]{};
  uint16_t nozzleMin = 0;
  uint16_t nozzleMax = 0;
  uint16_t bedMin = 0;
  uint16_t bedMax = 0;
  uint16_t diameterHundredthsMm = 0;
  uint16_t lengthMeters = 0;
  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
  String uidText;
};
