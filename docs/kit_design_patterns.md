# PM3 Kit Design Pattern Reference

This document maps shirt design numbers (0-9) to their visual patterns, reverse-engineered from the original PM3 game data.

## Design Pattern Numbers

| Design | Pattern Description | Example Clubs |
|--------|---------------------|---------------|
| **0** | Solid / Plain | Liverpool (all red), Manchester United (red), Chelsea (blue), Manchester City (sky blue), Tottenham (white), Bayern Munich, Real Madrid, Marseille, Everton |
| **1** | Vertical Stripes | Newcastle United (black/white), Juventus (black/white), Barcelona (red/blue), AC Milan (red/black), Southampton (red/white), Stoke City (red/white), Sunderland (red/white), Crystal Palace (red/blue), Sheffield United (red/white) |
| **2** | Sleeves/Shoulders | Arsenal (red with white sleeves), Rangers (blue), West Ham (can show hoops), Burnley, Watford |
| **3** | Solid with Trim/Piping | Galatasaray, Feyenoord, Walsall, Portsmouth (away), Burnley (away) |
| **4** | Halves (Vertical Split) | Blackburn Rovers (blue/white halves), West Ham (away) |
| **5** | Quarters/Quadrants | Bristol Rovers, Wycombe Wanderers, NK Rijeka |
| **6** | Thick Center Stripe | Ajax (white with red stripe), Liverpool (yellow away), Oldham (away), Hartlepool |
| **7** | Hoops | Celtic (green/white), QPR, Reading, CSKA Moscow, Sporting Lisbon, Ferencvaros |
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

## Pattern Usage Statistics

Based on analysis of all 244 clubs in the PM3 database:
- **Design 0 (Solid)**: ~50% of all kits - Most common
- **Design 1 (Stripes)**: ~20% of all kits - Very common for traditional clubs
- **Design 2 (Sleeves)**: ~15% of all kits - Arsenal's signature style
- **Design 7 (Hoops)**: ~5% of all kits - Celtic and traditional hooped teams
- **Designs 3-6**: ~8% of all kits - Specialty patterns
- **Designs 8-9**: <2% of all kits - Rare modern/gradient effects

## Notes

- The three most common patterns (0, 1, 2) account for 85% of all kits
- Design 1 is the classic choice for historic clubs with striped traditions
- Design 2 works for both sleeves (Arsenal) and can render hoops depending on colors
- Design 7 is specifically for traditional horizontal hoops (Celtic style)
- When `shirt_secondary` equals `shirt_primary`, the pattern renders as monochrome
- The secondary color determines the alternate color in stripes, hoops, sleeves, and halves

## How This Was Decoded

This reference was created by:
1. Exporting all 244 club kits from original PM3 data using `export_kit_data`
2. Cross-referencing design numbers with well-known club kits
3. Analyzing color combinations to understand pattern rendering

See `tools/export_kit_data.cpp` to export kit data from your own PM3 installation.
