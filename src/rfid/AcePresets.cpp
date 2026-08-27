#include "AcePresets.h"

#include <cstring>

#define P(n,m,c,h,s,n0,n1,b0,b1) {n,m,c,h,s,n0,n1,b0,b1,175,330,0,0,false,false}
#define V(n,m,c,h,s,n0,n1,b0,b1,len,a0,a1) {n,m,c,h,s,n0,n1,b0,b1,175,len,a0,a1,true,false}
#define C(n,m,c,h,s,n0,n1,b0,b1) {n,m,c,h,s,n0,n1,b0,b1,175,330,0,0,false,true}
const AcePreset AcePresets::ALL[] = {
 P("PLA Black","PLA","Black","212721FF","ACPLA-BLK",190,230,50,60),
 P("PLA White","PLA","White","EFF0F1FF","AHPLWH-101",190,230,50,60),
 P("PLA Grey","PLA","Grey","B1B3B3FF","ACPLA-GRY",190,230,50,60),
 P("PLA Red","PLA","Red","CE3845FF","ACPLA-RED",190,230,50,60),
 P("PLA Yellow","PLA","Yellow","F3E500FF","ACPLA-YEL",190,230,50,60),
 P("PLA Blue","PLA","Blue","003594FF","ACPLA-BLU",190,230,50,60),
 P("PLA Green","PLA","Green","009639FF","ACPLA-GRN",190,230,50,60),
 P("PLA Purple","PLA","Purple","6A6DCDFF","ACPLA-PUR",190,230,50,60),
 P("PLA Orange","PLA","Orange","FF7F32FF","ACPLA-ORG",190,230,50,60),
 P("PLA Pink","PLA","Pink","FF8DA1FF","ACPLA-PNK",190,230,50,60),
 P("PLA Green Flash","PLA","Green Flash","75CB5DFF","ACPLA-GFL",190,230,50,60),
 P("PLA Texture Grey","PLA","Texture Grey","75787BFF","ACPLA-TGY",190,230,50,60),
 P("PLA Beige","PLA","Beige","D4B996FF","ACPLA-BGE",190,230,50,60),
 P("PLA Bronze","PLA","Bronze","7C4D3AFF","ACPLA-BRZ",190,230,50,60),
 P("PLA Brown","PLA","Brown","927968FF","ACPLA-BRN",190,230,50,60),
 P("PLA Dark Brown","PLA","Dark Brown","975E3EFF","ACPLA-DBR",190,230,50,60),
 P("PLA Texture Silver","PLA","Texture Silver","8A8D8FFF","ACPLA-TSV",190,230,50,60),
 P("PLA Cyan","PLA","Cyan","23A3C7FF","ACPLA-CYN",190,230,50,60),
 P("PLA Magenta","PLA","Magenta","CF4F80FF","ACPLA-MAG",190,230,50,60),
 P("PLA Clear","PLA","Clear","FFFFFFFF","ACPLA-CLR",190,230,50,60),

 V("PLA Black Official","PLA","Black","212721FF","AHPLBK-107",190,230,55,65,330,50,200),
 V("PLA Texture Grey Official","PLA","Texture Grey","75787BFF","AHPLGY-106",190,230,55,65,330,50,200),

 V("PLA+ Orange Official","PLA+","Orange","FF7F32FF","AHPLPVO-108",190,230,55,65,330,50,200),

 C("PLA Basic Clear Catalog","PLA","Clear","FFFFFFFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic White Catalog","PLA","White","EFF0F1FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Mia White Catalog","PLA","Mia White","F1E9E0FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Mia Pink Catalog","PLA","Mia Pink","FAD6C6FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Peach Pink Catalog","PLA","Peach Pink","FFC196FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Yellow Catalog","PLA","Yellow","F3E500FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Gold Catalog","PLA","Gold","FFB81CFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Orange Catalog","PLA","Orange","FF7F32FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Pink Catalog","PLA","Pink","FF8DA1FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Magenta Catalog","PLA","Magenta","CF4F80FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Texture Red Catalog","PLA","Texture Red","EF3340FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Red Catalog","PLA","Red","CE3845FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Purple Catalog","PLA","Purple","6A6DCDFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Interstellar Violet Catalog","PLA","Interstellar Violet","5B618FFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Cyan Catalog","PLA","Cyan","23A3C7FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Tropical Turquoise Catalog","PLA","Tropical Turquoise","009CBDFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Blue Catalog","PLA","Blue","003594FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Spring Leaf Catalog","PLA","Spring Leaf","89A84FFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Olive Green Catalog","PLA","Olive Green","658946FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Green Catalog","PLA","Green","009639FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Beige Catalog","PLA","Beige","D4B996FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Brown Catalog","PLA","Brown","927968FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Bronze Catalog","PLA","Bronze","7C4D3AFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Light Grey Catalog","PLA","Light Grey","DAD9DBFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Grey Catalog","PLA","Grey","B1B3B3FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Texture Silver Catalog","PLA","Texture Silver","8A8D8FFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Texture Grey Catalog","PLA","Texture Grey","75787BFF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Blue Grey Catalog","PLA","Blue Grey","768692FF","AHPLBK-101",190,230,55,65),
 C("PLA Basic Black Catalog","PLA","Black","212721FF","AHPLBK-101",190,230,55,65),

 C("PLA+ Black Catalog","PLA+","Black","212721FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ White Catalog","PLA+","White","EFF0F1FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Bright Red Catalog","PLA+","Bright Red","E10600FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Brown Catalog","PLA+","Brown","927968FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Grey Catalog","PLA+","Grey","B1B3B3FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Texture Silver Catalog","PLA+","Texture Silver","8A8D8FFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Blue Catalog","PLA+","Blue","003594FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Texture Grey Catalog","PLA+","Texture Grey","75787BFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Beige Catalog","PLA+","Beige","D4B996FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Yellow Catalog","PLA+","Yellow","F3E500FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Tropical Turquoise Catalog","PLA+","Tropical Turquoise","009CBDFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Orange Catalog","PLA+","Orange","FF7F32FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Green Catalog","PLA+","Green","009639FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Green Flash Catalog","PLA+","Green Flash","75CB5DFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Pink Catalog","PLA+","Pink","FF8DA1FF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Purple Catalog","PLA+","Purple","6A6DCDFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Red Catalog","PLA+","Red","C8102EFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Spring Leaf Catalog","PLA+","Spring Leaf","89A84FFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Interstellar Violet Catalog","PLA+","Interstellar Violet","5B618FFF","AHPLPBK-102",190,230,55,65),
 C("PLA+ Peach Pink Catalog","PLA+","Peach Pink","FFC196FF","AHPLPBK-102",190,230,55,65),

 C("PLA HS Pearl Black Catalog","PLA","Pearl Black","212721FF","AHHSBK-103",190,260,55,65),
 C("PLA HS Texture Grey Catalog","PLA","Texture Grey","75787BFF","AHHSBK-103",190,260,55,65),
 C("PLA HS Bright White Catalog","PLA","Bright White","F1F0EDFF","AHHSBK-103",190,260,55,65),
 C("PLA HS Bright Red Catalog","PLA","Bright Red","E10600FF","AHHSBK-103",190,260,55,65),
 C("PLA HS Classic Green Catalog","PLA","Classic Green","3DB24EFF","AHHSBK-103",190,260,55,65),
 C("PLA HS Dazzling Blue Catalog","PLA","Dazzling Blue","3E55ABFF","AHHSBK-103",190,260,55,65),
 C("PLA HS Purple Opulence Catalog","PLA","Purple Opulence","695FA2FF","AHHSBK-103",190,260,55,65),
 C("PLA HS Vibrant Orange Catalog","PLA","Vibrant Orange","FF7338FF","AHHSBK-103",190,260,55,65),
 C("PLA HS Vibrant Yellow Catalog","PLA","Vibrant Yellow","FDDB27FF","AHHSBK-103",190,260,55,65),
 C("PLA HS Strawberry Pink Catalog","PLA","Strawberry Pink","F88192FF","AHHSBK-103",190,260,55,65),

 C("PLA Matte Black Catalog","PLA","Matte Black","484A49FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Dark Blue Catalog","PLA","Matte Dark Blue","375172FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Latte Brown Catalog","PLA","Matte Latte Brown","E2C6A5FF","HYGBK-102",190,230,60,65),
 C("PLA Matte White Catalog","PLA","Matte White","F1F0EDFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Desert Tan Catalog","PLA","Matte Desert Tan","DCCAA9FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Dark Green Catalog","PLA","Matte Dark Green","68724DFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Nardo Grey Catalog","PLA","Matte Nardo Grey","898B8EFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Bone White Catalog","PLA","Matte Bone White","EFEDE3FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Dark Brown Catalog","PLA","Matte Dark Brown","A38E80FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Red Catalog","PLA","Matte Red","DF484FFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Chocolate Brown Catalog","PLA","Matte Chocolate Brown","746759FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Orange Catalog","PLA","Matte Orange","FF9351FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Green Catalog","PLA","Matte Green","71C973FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Caramel Catalog","PLA","Matte Caramel","C4A981FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Yellow Catalog","PLA","Matte Yellow","FFE17FFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Sky Blue Catalog","PLA","Matte Sky Blue","7ECCEEFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Apple Green Catalog","PLA","Matte Apple Green","B9E972FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Blue Catalog","PLA","Matte Blue","1366A0FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Purple Catalog","PLA","Matte Purple","AE96D4FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Sakura Pink Catalog","PLA","Matte Sakura Pink","F4C1E1FF","HYGBK-102",190,230,60,65),
 C("PLA Matte Grey Catalog","PLA","Matte Grey","8E939BFF","HYGBK-102",190,230,60,65),
 C("PLA Matte Ice Blue Catalog","PLA","Matte Ice Blue","8AD8EDFF","HYGBK-102",190,230,60,65),

 C("PLA Silk Shiny Gold Catalog","Silk","Shiny Gold","F2A900FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Blue Catalog","Silk","Silk Blue","0067B9FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Christmas Green Catalog","Silk","Christmas Green","008755FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Silver Catalog","Silk","Silk Silver","8D9093FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Pink Catalog","Silk","Silk Pink","FCAFC0FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Christmas Red Catalog","Silk","Christmas Red","D22630FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Purple Catalog","Silk","Silk Purple","8B84D7FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Gold Catalog","Silk","Silk Gold","D19000FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Copper Catalog","Silk","Silk Copper","AF5C37FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk White Catalog","Silk","Silk White","E9E9E7FF","AHSCWH-102",210,240,55,65),
 C("PLA Silk Green Catalog","Silk","Silk Green","52D2BCFF","AHSCWH-102",210,240,55,65),

 C("PLA Marble Brick Red Catalog","PLA","Marble Brick Red","9D4815FF","AHPLM-001",200,230,55,65),
 C("PLA Marble White Catalog","PLA","Marble White","EFF0F1FF","AHPLM-001",200,230,55,65),
 P("PETG Black","PETG","Black","212721FF","ACPETG-BLK",230,250,60,70),
 P("PETG White","PETG","White","EFF0F1FF","AHPETGWH-101",230,250,60,70),
 P("PETG Grey","PETG","Grey","97999BFF","ACPETG-GRY",230,250,60,70),
 P("PETG Red","PETG","Red","C8102EFF","ACPETG-RED",230,250,60,70),
 P("PETG Yellow","PETG","Yellow","F3E500FF","ACPETG-YEL",230,250,60,70),
 P("PETG Blue","PETG","Blue","003594FF","ACPETG-BLU",230,250,60,70),
 P("PETG Green","PETG","Green","009639FF","ACPETG-GRN",230,250,60,70),
 P("PETG Purple","PETG","Purple","6A6DCDFF","ACPETG-PUR",230,250,60,70),
 P("PETG Orange","PETG","Orange","FF7F32FF","ACPETG-ORG",230,250,60,70),
 P("PETG Pink","PETG","Pink","FB637EFF","ACPETG-PNK",230,250,60,70),
 P("PETG Texture Grey","PETG","Texture Grey","75787BFF","ACPETG-TGY",230,250,60,70),
 P("PETG Texture Silver","PETG","Texture Silver","8A8D8FFF","ACPETG-TSV",230,250,60,70),
 P("PETG Dark Grey","PETG","Dark Grey","7E868AFF","ACPETG-DGY",230,250,60,70),
 P("PETG Brown","PETG","Brown","927968FF","ACPETG-BRN",230,250,60,70),
 P("PETG Beige","PETG","Beige","D4B996FF","ACPETG-BGE",230,250,60,70),
 P("PETG Peanut Brown","PETG","Peanut Brown","A9754FFF","ACPETG-PBN",230,250,60,70),
 P("PETG Cream","PETG","Cream","F9DFB9FF","ACPETG-CRM",230,250,60,70),
 P("PETG Lake Blue","PETG","Lake Blue","0084D4FF","ACPETG-LBU",230,250,60,70),
 P("PETG Forest Green","PETG","Forest Green","43523BFF","ACPETG-FGN",230,250,60,70),
 P("PETG Lime Green","PETG","Lime Green","78D64BFF","ACPETG-LGN",230,250,60,70),
 P("PETG Clear","PETG","Clear","FFFFFFFF","ACPETG-CLR",230,250,60,70),

 V("PETG Black Official","PETG","Black","212721FF","AHPEBK-102",230,250,60,70,320,50,200),
 V("PETG White Official","PETG","White","EFF0F1FF","AHPEBW-102",230,250,60,70,320,50,200),

 C("PETG Trans Blue Catalog","PETG","Translucent Blue","69B3E7FF","AHPETG-001",230,250,60,70),
 C("PETG Trans Grey Catalog","PETG","Translucent Grey","6A6C6EFF","AHPETG-001",230,250,60,70),
 C("PETG Trans Purple Catalog","PETG","Translucent Purple","D7A9E3FF","AHPETG-001",230,250,60,70),
 C("PETG Trans Brown Catalog","PETG","Translucent Brown","CDA077FF","AHPETG-001",230,250,60,70),
 C("PETG Trans Pink Catalog","PETG","Translucent Pink","F8C1B8FF","AHPETG-001",230,250,60,70),
 C("PETG Trans Green Catalog","PETG","Translucent Green","7CE0D3FF","AHPETG-001",230,250,60,70),
 C("PETG Trans Olive Catalog","PETG","Translucent Olive","78874DFF","AHPETG-001",230,250,60,70),
 C("PETG Trans Orange Catalog","PETG","Translucent Orange","FF8F1CFF","AHPETG-001",230,250,60,70),
 P("TPU Black","TPU","Black","212721FF","ACTPU-BLK",195,230,50,60),
 P("TPU Grey","TPU","Grey","63666AFF","ACTPU-GRY",195,230,50,60),
 P("TPU Milky White","TPU","Milky White","E9E9E7FF","ACTPU-MWH",195,230,50,60),
 P("TPU Red","TPU","Red","D22630FF","ACTPU-RED",195,230,50,60),
 P("TPU Orange","TPU","Orange","FF6A13FF","ACTPU-ORG",195,230,50,60),
 P("TPU Purple","TPU","Purple","A438A8FF","ACTPU-PUR",195,230,50,60),
 P("TPU Blue","TPU","Blue","005EB8FF","ACTPU-BLU",195,230,50,60),
 P("TPU Green","TPU","Clear Green","79C000FF","ACTPU-GRN",195,230,50,60),
 P("TPU Clear","TPU","Clear","FFFFFFFF","ACTPU-CLR",195,230,50,60),
 P("ABS Black","ABS","Black","212721FF","ACABS-BLK",240,260,90,100),
 P("ABS White","ABS","White","ECECE7FF","ACABS-WHT",240,260,90,100),
 P("ABS Grey","ABS","Grey","A7A8AAFF","ACABS-GRY",240,260,90,100),
 P("ABS Red","ABS","Red","D6001CFF","ACABS-RED",240,260,90,100),
 P("ABS Orange","ABS","Orange","FF671FFF","ACABS-ORG",240,260,90,100),
 P("ABS Yellow","ABS","Yellow","FFE900FF","ACABS-YEL",240,260,90,100),
 P("ABS Green","ABS","Green","00B140FF","ACABS-GRN",240,260,90,100),
 P("ABS Blue","ABS","Blue","00239CFF","ACABS-BLU",240,260,90,100),

 C("TPU 95A Black Catalog","TPU","Black","212721FF","STPBK-101",195,230,50,60),
 C("TPU 95A Clear Blue Catalog","TPU","Clear Blue","005EB8FF","STPBK-101",195,230,50,60),
 C("TPU 95A Milky White Catalog","TPU","Milky White","E9E9E7FF","STPBK-101",195,230,50,60),
 C("TPU 95A Clear Red Catalog","TPU","Clear Red","D22630FF","STPBK-101",195,230,50,60),
 C("TPU 95A Grey Catalog","TPU","Grey","63666AFF","STPBK-101",195,230,50,60),
 C("TPU 95A Clear Catalog","TPU","Clear","FFFFFFFF","STPBK-101",195,230,50,60),

 C("ABS Black Catalog","ABS","Black","212721FF","SHABBK-102",240,280,80,100),
 C("ABS Grey Catalog","ABS","Grey","A7A8AAFF","SHABBK-102",240,280,80,100),
 C("ABS Texture Grey Catalog","ABS","Texture Grey","75787BFF","SHABBK-102",240,280,80,100),
 C("ABS White Catalog","ABS","White","ECECE7FF","SHABBK-102",240,280,80,100),
 C("ABS Red Catalog","ABS","Red","D6001CFF","SHABBK-102",240,280,80,100),
 C("ABS Green Catalog","ABS","Green","00B140FF","SHABBK-102",240,280,80,100),
 C("ABS Orange Catalog","ABS","Orange","FF671FFF","SHABBK-102",240,280,80,100),
 C("ABS Blue Catalog","ABS","Blue","00239CFF","SHABBK-102",240,280,80,100),
 C("ABS Yellow Catalog","ABS","Yellow","FFE900FF","SHABBK-102",240,280,80,100),

 C("ASA Black Catalog","ASA","Black","212721FF","AHASA-001",255,275,80,100),
 C("ASA Red Catalog","ASA","ASA Red","EE2737FF","AHASA-001",255,275,80,100),
 C("ASA Grey Catalog","ASA","Texture Grey","75787BFF","AHASA-001",255,275,80,100),
 C("ASA Ivory Catalog","ASA","Ivory","EFEDE3FF","AHASA-001",255,275,80,100),

 C("PC Black Catalog","PC","PC Black","212322FF","AHPC-001",270,290,100,120),
 C("PC White Catalog","PC","PC White","FFFFFFFF","AHPC-001",270,290,100,120)
};
#undef P
#undef V
#undef C

const uint8_t AcePresets::COUNT = sizeof(AcePresets::ALL) / sizeof(AcePresets::ALL[0]);
const char *const AcePresets::MATERIALS[] = {"PLA", "PLA+", "PETG", "TPU", "OTHER"};
const uint8_t AcePresets::MATERIAL_COUNT = 5;

uint8_t AcePresets::countForMaterial(uint8_t materialIndex) {
  uint8_t count = 0;
  if (materialIndex >= MATERIAL_COUNT) return 0;
  auto matches = [materialIndex](const char *material) {
    if (materialIndex < 4) return !strcmp(material, MATERIALS[materialIndex]);
    return strcmp(material, "PLA") && strcmp(material, "PLA+") &&
           strcmp(material, "PETG") && strcmp(material, "TPU");
  };
  for (uint8_t i = 0; i < COUNT; ++i) if (matches(ALL[i].material)) ++count;
  return count;
}

int16_t AcePresets::globalIndex(uint8_t materialIndex, uint8_t filteredIndex) {
  if (materialIndex >= MATERIAL_COUNT) return -1;
  auto matches = [materialIndex](const char *material) {
    if (materialIndex < 4) return !strcmp(material, MATERIALS[materialIndex]);
    return strcmp(material, "PLA") && strcmp(material, "PLA+") &&
           strcmp(material, "PETG") && strcmp(material, "TPU");
  };
  for (uint8_t i = 0, found = 0; i < COUNT; ++i) {
    if (!matches(ALL[i].material)) continue;
    if (found++ == filteredIndex) return i;
  }
  return -1;
}

namespace {
void putString(AceTagData &tag, uint8_t page, const char *text, uint8_t maxBytes) {
  uint8_t buffer[20]{};
  const size_t length = min(static_cast<size_t>(maxBytes), strlen(text));
  memcpy(buffer, text, length);
  for (uint8_t i = 0; i < maxBytes / 4; ++i) memcpy(tag.pages[page + i], buffer + i * 4, 4);
}
void putPair(uint8_t out[4], uint16_t a, uint16_t b) {
  out[0] = a; out[1] = a >> 8; out[2] = b; out[3] = b >> 8;
}
uint8_t hexNibble(char c) { return c <= '9' ? c - '0' : 10 + (c - 'A'); }
}

void AcePresets::buildTag(const AcePreset &p, AceTagData &tag) {
  tag = AceTagData{};
  tag.pages[4][0] = 0x7B; tag.pages[4][2] = 0x65;
  putString(tag, 5, p.sku, 20);
  putString(tag, 10, "AC", 20);
  putString(tag, 15, p.material, 20);
  uint8_t rgba[4];
  for (uint8_t i = 0; i < 4; ++i) rgba[i] = (hexNibble(p.colorHex[i * 2]) << 4) | hexNibble(p.colorHex[i * 2 + 1]);
  for (uint8_t i = 0; i < 4; ++i) tag.pages[20][i] = rgba[3 - i];
  if (p.auxiliaryMin || p.auxiliaryMax) putPair(tag.pages[23], p.auxiliaryMin, p.auxiliaryMax);
  putPair(tag.pages[24], p.nozzleMin, p.nozzleMax);
  putPair(tag.pages[29], p.bedMin, p.bedMax);
  putPair(tag.pages[30], p.diameter, p.lengthMeters);
  tag.pages[31][0] = 0xE8; tag.pages[31][1] = 0x03;
  tag.readOk = true;
}
