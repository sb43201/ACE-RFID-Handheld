# ACE tag format used by ace-rfid-box

Pages are four bytes each. The handheld reads and preserves pages 4–31.

| Pages | Meaning |
|---|---|
| 4 | Header `7B 00 65 00` |
| 5–9 | SKU, 20-byte NUL-padded ASCII |
| 10–14 | Brand/source, 20-byte NUL-padded ASCII |
| 15–19 | Material, 20 bytes written; original reader uses first 16 |
| 20 | RRGGBBAA stored in reverse byte order |
| 21–23 | Reserved/zero in generated tags |
| 24 | Nozzle minimum and maximum, two little-endian uint16 values |
| 25–28 | Reserved/zero in generated tags |
| 29 | Bed minimum and maximum, two little-endian uint16 values |
| 30 | Diameter ×100 and length in meters, little-endian uint16 values |
| 31 | Fixed footer `E8 03 00 00` (1000 little-endian) |

Stage 3 validation requires the exact header and footer, printable non-empty SKU/brand/material fields, and structurally plausible numeric values. Color names are looked up from the original preset table because only the RGBA value is stored on the tag.

