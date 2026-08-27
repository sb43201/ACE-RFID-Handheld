#pragma once

#include "AceTag.h"

struct AcePreset {
  const char *name;
  const char *material;
  const char *colorName;
  const char *colorHex;
  const char *sku;
  uint16_t nozzleMin, nozzleMax, bedMin, bedMax, diameter, lengthMeters;
  uint16_t auxiliaryMin, auxiliaryMax;
  bool factoryVerified;
  bool catalogDerived;
};

namespace AcePresets {
extern const AcePreset ALL[];
extern const uint8_t COUNT;
extern const char *const MATERIALS[];
extern const uint8_t MATERIAL_COUNT;
uint8_t countForMaterial(uint8_t materialIndex);
int16_t globalIndex(uint8_t materialIndex, uint8_t filteredIndex);
void buildTag(const AcePreset &preset, AceTagData &tag);
}
