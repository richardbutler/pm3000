# PM3 Kit Design Pattern Reference

This document maps shirt design numbers (0-9) to their visual patterns, reverse-engineered from the original PM3 game data.

## Design Pattern Numbers

| Design | Pattern Description | Example Clubs |
|--------|---------------------|---------------|
| **0** | Solid / Plain | Liverpool (all red), Manchester United (all red), Everton (all blue) |
| **1** | Vertical Stripes | Newcastle (black/white), Southampton (red/white), Stoke (red/white), Sunderland (red/white), Crystal Palace (red/blue), Sheffield United (red/white), Brentford (red/white) |
| **2** | Horizontal Hoops | Arsenal (red with white), West Ham (claret/blue), Burnley (claret/blue), Watford (yellow/red), Derby (white/black) |
| **3** | Solid with Trim/Piping | Walsall, Portsmouth (away), Burnley (away), Feyenoord, Galatasaray |
| **4** | Halves (Vertical Split) | Blackburn Rovers (blue/white halves), West Ham (away) |
| **5** | Quarters/Quadrants | Bristol Rovers, Wycombe Wanderers, NK Rijeka |
| **6** | Thick Center Stripe | Ajax (white with red stripe), Liverpool (away), Oldham (away), Hartlepool |
| **7** | Hoops (Thicker Style) | QPR, Reading |
| **8** | Gradient/Fade | Arsenal (away), Sheffield United (away), Utrecht, Croatia Zagreb |
| **9** | Diagonal/Sash | Hadjuk Split, NK Svoboda, Flamurtari |

## Color Values

Colors use 4-bit RGB (0-15 per channel):
- **0** = Black
- **1** = Dark Blue
- **8** = Light Blue/Sky Blue
- **11** = Red/Burgundy
- **13** = White
- **15** = Yellow/Bright

The `shirt_secondary` color is used for the secondary element in patterns (stripes, hoops, etc.)

## Kit Structure

Each club has 3 kits (indices 0-2):
- **Kit 0**: Home
- **Kit 1**: Away 1
- **Kit 2**: Away 2

Each kit contains:
- `shirt_design` (0-9): Pattern type
- `shirt_primary_r/g/b`: Primary shirt color (4-bit RGB)
- `shirt_secondary_r/g/b`: Secondary shirt color for patterns (4-bit RGB)
- `shorts_r/g/b`: Shorts color (4-bit RGB)
- `socks_r/g/b`: Socks color (4-bit RGB)

## Notes

- Design patterns 0-2 are most common (75% of all kits)
- Design 7 appears mainly in English clubs
- Designs 8-9 are rare and may represent special/custom patterns
- When `shirt_secondary` equals `shirt_primary`, the pattern is monochrome
- The secondary color determines the alternate color in stripes, hoops, and halves

## How This Was Decoded

This reference was created by:
1. Exporting all 244 club kits from original PM3 data using `export_kit_data`
2. Cross-referencing design numbers with well-known club kits
3. Analyzing color combinations to understand pattern rendering

See `tools/export_kit_data.cpp` to export kit data from your own PM3 installation.
