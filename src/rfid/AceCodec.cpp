#include "AceCodec.h"
#include "AcePresets.h"

#include <TFT_eSPI.h>

namespace {
struct ColorEntry { const char *material; const char *hex; const char *name; };

// Human-readable color names are not stored on the tag. This table is ported
// from ace-rfid-box and is therefore the authoritative name lookup.
constexpr ColorEntry COLORS[] = {
  {"PLA","212721FF","Black"},{"PLA","EFF0F1FF","White"},{"PLA","B1B3B3FF","Grey"},
  {"PLA","CE3845FF","Red"},{"PLA","F3E500FF","Yellow"},{"PLA","003594FF","Blue"},
  {"PLA","009639FF","Green"},{"PLA","6A6DCDFF","Purple"},{"PLA","FF7F32FF","Orange"},
  {"PLA","FF8DA1FF","Pink"},{"PLA","75CB5DFF","Green Flash"},{"PLA","75787BFF","Texture Grey"},
  {"PLA","D4B996FF","Beige"},{"PLA","7C4D3AFF","Bronze"},{"PLA","927968FF","Brown"},
  {"PLA","975E3EFF","Dark Brown"},{"PLA","8A8D8FFF","Texture Silver"},{"PLA","23A3C7FF","Cyan"},
  {"PLA","CF4F80FF","Magenta"},{"PLA","FFFFFFFF","Clear"},
  {"PETG","212721FF","Black"},{"PETG","EFF0F1FF","White"},{"PETG","97999BFF","Grey"},
  {"PETG","C8102EFF","Red"},{"PETG","F3E500FF","Yellow"},{"PETG","003594FF","Blue"},
  {"PETG","009639FF","Green"},{"PETG","6A6DCDFF","Purple"},{"PETG","FF7F32FF","Orange"},
  {"PETG","FB637EFF","Pink"},{"PETG","75787BFF","Texture Grey"},{"PETG","8A8D8FFF","Texture Silver"},
  {"PETG","7E868AFF","Dark Grey"},{"PETG","927968FF","Brown"},{"PETG","D4B996FF","Beige"},
  {"PETG","A9754FFF","Peanut Brown"},{"PETG","F9DFB9FF","Cream"},{"PETG","0084D4FF","Lake Blue"},
  {"PETG","43523BFF","Forest Green"},{"PETG","78D64BFF","Lime Green"},{"PETG","FFFFFFFF","Clear"},
  {"TPU","212721FF","Black"},{"TPU","63666AFF","Grey"},{"TPU","E9E9E7FF","Milky White"},
  {"TPU","D22630FF","Red"},{"TPU","FF6A13FF","Orange"},{"TPU","A438A8FF","Purple"},
  {"TPU","005EB8FF","Blue"},{"TPU","79C000FF","Clear Green"},{"TPU","FFFFFFFF","Clear"},
  {"ABS","212721FF","Black"},{"ABS","ECECE7FF","White"},{"ABS","A7A8AAFF","Grey"},
  {"ABS","D6001CFF","Red"},{"ABS","FF671FFF","Orange"},{"ABS","FFE900FF","Yellow"},
  {"ABS","00B140FF","Green"},{"ABS","00239CFF","Blue"}
};

uint16_t little16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

bool copyAscii(const uint8_t *source, size_t sourceLength, char *dest, size_t destSize) {
  size_t out = 0;
  bool seenTerminator = false;
  for (size_t i = 0; i < sourceLength; ++i) {
    const uint8_t c = source[i];
    if (c == 0 || c == 0xFF) { seenTerminator = true; continue; }
    if (seenTerminator || c < 32 || c > 126 || out + 1 >= destSize) return false;
    dest[out++] = static_cast<char>(c);
  }
  dest[out] = '\0';
  return out > 0;
}

const char *findColor(const char *sku, const char *material, const char *hex) {
  // SKU disambiguates catalog families that share PLA material bytes and the
  // same RGB value (for example Basic Black versus High Speed Pearl Black).
  for (uint8_t i = 0; i < AcePresets::COUNT; ++i)
    if (!strcmp(AcePresets::ALL[i].sku, sku) &&
        !strcmp(AcePresets::ALL[i].material, material) &&
        !strcmp(AcePresets::ALL[i].colorHex, hex)) return AcePresets::ALL[i].colorName;
  for (const auto &entry : COLORS)
    if (!strcmp(entry.material, material) && !strcmp(entry.hex, hex)) return entry.name;
  for (uint8_t i = 0; i < AcePresets::COUNT; ++i)
    if (!strcmp(AcePresets::ALL[i].material, material) &&
        !strcmp(AcePresets::ALL[i].colorHex, hex)) return AcePresets::ALL[i].colorName;
  for (const auto &entry : COLORS)
    if (!strcmp(entry.hex, hex)) return entry.name;
  for (uint8_t i = 0; i < AcePresets::COUNT; ++i)
    if (!strcmp(AcePresets::ALL[i].colorHex, hex)) return AcePresets::ALL[i].colorName;
  return "Unknown color";
}
}  // namespace

bool AceCodec::validateAndDecode(AceTagData &tag) {
  tag.aceValid = false;
  if (!tag.readOk) return false;

  // Exact constants emitted by ace-rfid-box buildPresetTag().
  const uint8_t *header = tag.pages[4];
  const uint8_t *footer = tag.pages[31];
  if (header[0] != 0x7B || header[1] != 0x00 || header[2] != 0x65 || header[3] != 0x00 ||
      footer[0] != 0xE8 || footer[1] != 0x03 || footer[2] != 0x00 || footer[3] != 0x00) return false;

  if (!copyAscii(&tag.pages[5][0], 20, tag.sku, sizeof(tag.sku)) ||
      !copyAscii(&tag.pages[10][0], 20, tag.brand, sizeof(tag.brand)) ||
      !copyAscii(&tag.pages[15][0], 16, tag.material, sizeof(tag.material))) return false;

  tag.nozzleMin = little16(&tag.pages[24][0]);
  tag.nozzleMax = little16(&tag.pages[24][2]);
  tag.bedMin = little16(&tag.pages[29][0]);
  tag.bedMax = little16(&tag.pages[29][2]);
  tag.diameterHundredthsMm = little16(&tag.pages[30][0]);
  tag.lengthMeters = little16(&tag.pages[30][2]);
  if (tag.nozzleMin < 100 || tag.nozzleMax > 400 || tag.nozzleMin > tag.nozzleMax ||
      tag.bedMax > 200 || tag.bedMin > tag.bedMax || tag.diameterHundredthsMm < 50 ||
      tag.diameterHundredthsMm > 500 || tag.lengthMeters == 0 || tag.lengthMeters > 5000) return false;

  // Page 20 is the four RRGGBBAA bytes stored in reverse byte order.
  snprintf(tag.colorHex, sizeof(tag.colorHex), "%02X%02X%02X%02X",
           tag.pages[20][3], tag.pages[20][2], tag.pages[20][1], tag.pages[20][0]);
  tag.red = tag.pages[20][3]; tag.green = tag.pages[20][2]; tag.blue = tag.pages[20][1];
  strlcpy(tag.colorName, findColor(tag.sku, tag.material, tag.colorHex), sizeof(tag.colorName));
  if (!strcmp(tag.colorName, "Unknown color")) strlcpy(tag.name, tag.material, sizeof(tag.name));
  else snprintf(tag.name, sizeof(tag.name), "%s %s", tag.material, tag.colorName);
  tag.aceValid = true;
  return true;
}

void AceCodec::dump(const AceTagData &tag) {
  Serial.println("[ace] Raw pages:");
  for (uint8_t page = 4; page <= 31; ++page)
    Serial.printf("[ace] P%02u: %02X %02X %02X %02X\n", page, tag.pages[page][0],
                  tag.pages[page][1], tag.pages[page][2], tag.pages[page][3]);
  Serial.printf("[ace] Valid: %s\n", tag.aceValid ? "YES" : "NO");
  if (!tag.aceValid) return;
  Serial.printf("[ace] Name: %s\n[ace] Material: %s\n[ace] SKU: %s\n", tag.name, tag.material, tag.sku);
  Serial.printf("[ace] Color: %s\n[ace] ColorHex: %s\n[ace] RGB: %u,%u,%u\n",
                tag.colorName, tag.colorHex, tag.red, tag.green, tag.blue);
  Serial.printf("[ace] Nozzle: %u-%u C\n[ace] Bed: %u-%u C\n", tag.nozzleMin, tag.nozzleMax, tag.bedMin, tag.bedMax);
  Serial.printf("[ace] Diameter: %.2f mm\n[ace] Length: %u m\n",
                tag.diameterHundredthsMm / 100.0f, tag.lengthMeters);
}

uint16_t AceCodec::rgb565(const AceTagData &tag) {
  return static_cast<uint16_t>(((tag.red & 0xF8) << 8) | ((tag.green & 0xFC) << 3) | (tag.blue >> 3));
}

uint16_t AceCodec::contrastText(const AceTagData &tag) {
  const uint16_t luminance = (299UL * tag.red + 587UL * tag.green + 114UL * tag.blue) / 1000UL;
  return luminance >= 150 ? TFT_BLACK : TFT_WHITE;
}
