# Anycubic catalog-derived presets

Stage 5.1 includes 123 `CATALOG` presets derived from the current Anycubic filament catalog and public ACE material-family mappings.

## Included families

| Family | Colors | Encoded material | Family SKU used | Nozzle | Bed |
|---|---:|---|---|---|---|
| PLA Basic | 29 | PLA | AHPLBK-101 | 190-230 C | 55-65 C |
| PLA+ | 20 | PLA+ | AHPLPBK-102 | 190-230 C | 55-65 C |
| PLA High Speed | 10 | PLA | AHHSBK-103 | 190-260 C | 55-65 C |
| PLA Matte | 22 | PLA | HYGBK-102 | 190-230 C | 60-65 C |
| PLA Silk | 11 | Silk | AHSCWH-102 | 210-240 C | 55-65 C |
| PLA Marble | 2 | PLA | AHPLM-001 | 200-230 C | 55-65 C |
| PETG Translucent | 8 | PETG | AHPETG-001 | 230-250 C | 60-70 C |
| TPU 95A | 6 | TPU | STPBK-101 | 195-230 C | 50-60 C |
| ABS | 9 | ABS | SHABBK-102 | 240-280 C | 80-100 C |
| ASA | 4 | ASA | AHASA-001 | 255-275 C | 80-100 C |
| PC | 2 | PC | AHPC-001 | 270-290 C | 100-120 C |

The database contains 186 total presets: 58 original community presets, 5 factory-verified captures, and 123 catalog-derived presets.

## Confidence and limitations

`CATALOG` means the color hex and temperature ranges come from current official Anycubic product listings, while material bytes and family SKU patterns come from the public Anycubic ACE RFID editor mapping. These are not factory RFID page captures.

- Anycubic does not publish the exact per-color RFID SKU for most catalog colors. A documented family SKU is therefore reused within each family.
- Diameter is 1.75 mm. Length remains the legacy ACE writer default of 330 m because catalog pages publish mass rather than encoded RFID length.
- Page 23 remains zero for catalog presets because its bytes are not published for these families.
- The printer may recognize material and color without treating a catalog-derived tag exactly like an official factory tag.

## Deliberately excluded

- Rainbow, dual-color, and tri-color products: one ACE color field cannot represent multiple colors accurately.
- Glow variants lacking an official single color hex.
- ABS, TPU, and PC variants whose catalog listing omits a hex value.
- PLA Galaxy and PLA Metal: official colors/settings exist, but no authoritative RFID family SKU mapping was found.
- Carbon-fiber, PA6-CF, PET-CF, PVA, and other families without a sufficiently documented ACE RFID material/SKU mapping.

These entries should be added only after an actual factory tag capture or another byte-level authoritative source becomes available.

## Sources

- https://store.anycubic.com/collections/filaments
- https://store.anycubic.com/products/pla-basic-mix-match-deal
- https://github.com/OrochW/Anycubic-ACE-RFID-Tool
