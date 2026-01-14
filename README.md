# PM3000 - Premier Manager 3 Game Add-on

PM3000 is an extension for Premier Manager 3 (Standard or Deluxe) that lets you load your save games and add extra tooling on top of the classic DOS experience.

You must already own an original copy of Premier Manager 3 or Premier Manager 3 Deluxe to use this project. PM3000 reads and manipulates the existing GAMEA/GAMEB/GAMEC files from that installation so it cannot run standalone.

The new features are intended to overcome some of the game's annoyances.
- Change Team - This screen allows you to switch to a new team, unlocking the ability to start the game as your favorite team.
- View Squad - This screen shows your current team for ease of visibility
- Scout - A scout that can see the stats of any player
- Free Players - This screen shows all of the out-of-contract players
- Convert Player to Coach - This screen allows you to retire a player and convert them into a coach
- Telephone - This adds a few new features:
  - Advertise for fans - Run an ad campaign and increase your fan base, leading to more attendance
  - Entertain team - Take the team for a night out and boost their morale
  - Arrange a training camp - Increase the stats of your players
  - Appeal red card - Ask the FA to overturn a player ban
  - Build new stadium - Save time by building a whole new stadium

This has been tested with Premier Manager 3 running under DOSBox on modern systems. The Amiga version has not been tested, but the data formats are shared, so the UI should still work when pointed at an Amiga save folder.

## Screenshots
![Loading Screen](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/loading.png)
![Load Game](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/load-game.png)
![Change Team](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/change-team.png)
![Free Players](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/free-players.png)
![Convert Coach](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/convert-coach.png)
![Telephone](https://raw.githubusercontent.com/martinbutt/pm3000/refs/heads/main/docs/screenshots/telephone.png)

## Building

### Prerequisites

**On Debian/Ubuntu based distributions, use the following command:**

```sh
sudo apt install git build-essential pkg-config cmake cmake-data libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libgtk-3-dev
```

**On macOS, install the SDL2/SDL2_image/SDL2_ttf bottles so the release binary can load those dylibs from `/opt/homebrew/opt`.**

```sh
brew install sdl2 sdl2_image sdl2_ttf
```

`gtk+3` is only required when building on Linux (see the Debian/Ubuntu instructions).

If you want to build from source locally, also install the build tooling before running `cmake`/`ninja`:

```sh
brew install git cmake ninja
```

### Instructions

```sh
# Clone this repo
git clone https://gitlab.com/martinbutt/pm3000.git
cd pm3000

# Create a build folder
mkdir build
cd build

# Build
cmake ..
make

# Run
./pm3000
```

### Tests

After configuring, you can build and run the lightweight unit tests (currently covering PM3 utility pricing logic):

```sh
# From the repo root (after cmake ..)
cmake --build build --target pm3_utils_tests
cd build && ctest --output-on-failure
```

Add more cases under `tests/` as you extend the utilities.

Backups of the three PM3 game files (`gamedata.dat`, `clubdata.dat`, `playdata.dat`) or save files (`saves/game*`) are made automatically in the `PM3000/` folder inside the PM3 save directory before any import or save mutation runs.

### FIFA import tool

The FIFA import tool reads a CSV export (e.g. `external/FC26_YYYYMMDD.csv`) and updates `gamedata.dat`, `clubdata.dat`, and `playdata.dat` for English leagues only. It preserves National League clubs (tier 5), caps squads at 16 players per club, and will generate two Premier League clubs if only 20 are present in the CSV.

```sh
# Build the tool
cmake --build build --target fifa_import_tool

# Import into save slot 1
./build/fifa_import_tool --csv external/FC26_20250921.csv --pm3 /path/to/PM3 --game 1 --year 2025
```

Optional flags:
- `--import-loans` enables loan period data (see TODO: game currently overwrites loan flags on startup).
- `--player-id <id>` imports a single CSV player by id for debugging.
- `--debug-player <id>` prints mapping details for one player id.
- `--verify-gamedata` checks a read/write roundtrip of `gamedata.dat`.
- `--dropped-clubs <path>` writes the tier-4 clubs dropped into a CSV.

> **Note:** If you run with `--base --import-loans`, the tool will warn that the game currently overwrites loan flags on startup with player bans.

Required CSV columns:
```
player_id
short_name
overall
player_positions
age
preferred_foot
club_name
club_loaned_from
league_name
league_level
league_id
pace
shooting
passing
dribbling
defending
physic
attacking_heading_accuracy
skill_ball_control
mentality_aggression
mentality_composure
goalkeeping_diving
goalkeeping_handling
goalkeeping_kicking
goalkeeping_positioning
goalkeeping_reflexes
goalkeeping_speed
attacking_crossing
attacking_finishing
attacking_short_passing
attacking_volleys
skill_dribbling
skill_curve
skill_fk_accuracy
skill_long_passing
movement_acceleration
movement_sprint_speed
movement_agility
movement_reactions
movement_balance
power_shot_power
power_jumping
power_stamina
power_strength
power_long_shots
mentality_interceptions
mentality_positioning
mentality_vision
mentality_penalties
defending_marking_awareness
defending_standing_tackle
defending_sliding_tackle
wage_eur
club_contract_valid_until_year
```

### FIFA Full Import Tool (244 teams with custom kits)

The **FIFA Full Import Tool** accepts separate clubs and players CSV files to import all 244 teams into PM3 with complete customization including kit colors, stadiums, and managers. Unlike the standard FIFA tool, this replaces all teams (no matching) and supports multi-league imports beyond just English football.

```sh
# Build the tool
cmake --build build --target fifa_import_full_tool

# Import into save slot 1
./build/fifa_import_full_tool --clubs teams.csv --players players.csv --pm3 /path/to/PM3 --game 1 --year 2025 --verbose
```

#### Clubs CSV Format

See `docs/csv_format_clubs.md` for full details. Required columns:

```
club_id,club_name,manager_name,stadium_name,league,max_players,
home_shirt_design,home_shirt_primary_r,home_shirt_primary_g,home_shirt_primary_b,
home_shirt_secondary_r,home_shirt_secondary_g,home_shirt_secondary_b,
home_shorts_r,home_shorts_g,home_shorts_b,
home_socks_r,home_socks_g,home_socks_b
```

Note: `max_players` is optional - omit or set to 0 to use the global `--max-players` default.

Optional away kit columns: `away1_*` and `away2_*` (same pattern as home kit).

#### Players CSV Format

See `docs/csv_format_players.md` for full details. Required columns include:

```
player_id,club_id,short_name,age,player_positions,overall,preferred_foot,
pace,shooting,passing,dribbling,defending,physic,attacking_heading_accuracy,
goalkeeping_diving,goalkeeping_handling,goalkeeping_kicking,goalkeeping_positioning,
goalkeeping_reflexes,goalkeeping_speed
```

Plus optional detailed FIFA stats (crossing, finishing, etc.) and contract data.

#### Key Features

- **244-team support**: Import all available club slots
- **Custom kit colors**: RGB colors (0-15) for shirts, shorts, socks with design patterns
- **Full club metadata**: Names (16 chars), managers (16 chars), stadiums (24 chars)
- **League assignment**: Place clubs in tiers 0-4 (Premier through Conference)
- **No English-only filter**: Import any league structure
- **Player linking**: Players CSV links to clubs via `club_id` foreign key
- **Flexible squad sizes**: Per-club `max_players` column (1-24) or global default via `--max-players`
- **Top-N selection**: Imports top players by rating for each club

#### Optional Flags

- `--max-players <N>`: Default players per club when `max_players` column is omitted (1-24, default 16)
- `--import-loans`: Enable loan period tracking
- `--base` or `--default`: Modify default game data instead of a save slot
- `--verbose` or `-v`: Show detailed import progress (includes per-club squad sizes)

#### Example Workflow

1. **Export data** from your source (spreadsheet, database, FIFA tool)
2. **Format as two CSVs**: One for clubs, one for players
3. **Link via club_id**: Each player row references a club
4. **Ensure each club has at least 2 goalkeepers** - import will fail otherwise
5. **Run import**: Tool replaces all 244 teams in order
6. **Start game**: Teams appear with custom kits and squads

#### Color Reference

Kit colors use 4-bit RGB (0-15 per channel):
- Red: `15,0,0`
- Blue: `0,0,15`
- White: `15,15,15`
- Black: `0,0,0`
- Yellow: `15,15,0`
- Green: `0,15,0`

Shirt designs: `0`=solid, `1`=vertical stripes, `2`=hoops, `3`=trim, `4`=halves, `5`=quarters, `6`=center stripe, `7`=thick hoops, `8`=gradient, `9`=sash. See [docs/kit_design_patterns.md](docs/kit_design_patterns.md) for complete reference with example clubs.

### SWOS team import tool

The repo now vendors the SWOS `TEAM.008` parser directly (no external checkout needed). A CLI helper ships with the build to import SWOS teams/players into a PM3 save:

```sh
# Build the tool
cmake --build build --target swos_import_tool

# Import TEAM.008 into save slot 1 for a PM3 folder
./build/swos_import_tool --team /path/to/TEAM.008 --pm3 /path/to/PM3 --game 1
```

The same functionality is now exposed from inside the SDL UI—use the Settings screen's **Import SWOS Teams** entry, which will re-use the currently configured PM3 folder and prompt for a TEAM.xxx file.

Before each import (CLI or UI) the tool copies `gamedata.dat`, `clubdata.dat`, and `playdata.dat` into the `PM3000/` backup directory within the selected PM3 folder.

What it does:
- Matches imported teams to existing GAMEB clubs by name and updates league/manager/kit, renaming players role-for-role.
- Any imported teams not found replace unmatched clubs, assign a generic stadium name, copy kit colors, and rename squad players in place.
> **Note:** SWOS and PM3 have different league structures now, so the import is a best effort—team counts per division still differ and some placements/kits get guessed where the source data no longer lines up perfectly.

#### Refreshing `TEAM.008` from SWOS2020

`TEAM.008` is the Sensible World of Soccer squad file that the importer reads. The SWOS2020 project keeps a curated, up-to-date version:

1. Download the latest package from https://swos2020.com/downloads. Pick the `.zip`/`.lha` bundle for the team database.
2. Extract the archive with your favorite tool (`lha`, `7z`, etc.) until you find `TEAM.008`.
3. Pass that extracted file to `swos_import_tool --team` to import the newest SWOS rosters into your PM3 save.

## Inspecting PM3 data files

`inspect_pm3_data` is now a lightweight dumper that reads `gamedata.dat` and prints the core `gamea` records as plain text for debugging.

```sh
# Build the tool
cmake --build build --target inspect_pm3_data

# Dump the gamedata summary
./build/inspect_pm3_data --pm3 /path/to/PM3
```

This outputs every `club_index`, `top_scorer`, and league table entry in a basic, predictable format that can later be grepped or diffed while keeping manual edits minimal.

## Exporting kit data

`export_kit_data` reads `clubdata.dat` and exports all club kit information including design patterns and colors. This is useful for reverse-engineering what shirt design numbers correspond to which patterns.

```sh
# Build the tool
g++ -std=c++17 -I src -I include tools/export_kit_data.cpp -o build/export_kit_data

# Export all club kits in human-readable format
./build/export_kit_data --pm3 /path/to/PM3

# Export as CSV for analysis
./build/export_kit_data --pm3 /path/to/PM3 --csv > kits.csv
```

**Reverse-engineering kit design patterns:**

By exporting kit data from the original PM3 game files, you can identify what design numbers correspond to which patterns by examining well-known clubs:

- Arsenal: Red shirt with white sleeves (likely design value for "sleeves" pattern)
- Blackburn: Blue and white halves (likely design value for "halves" pattern)
- Celtic: Green and white hoops (design value 2 for "hoops")
- Newcastle: Black and white stripes (design value 1 for "vertical stripes")

Once you export the data, cross-reference the design values with the visual appearance of clubs you know to build a complete mapping of design numbers to patterns.

## Acknowledgements
Special thanks to [@eb4x](https://www.github.com/eb4x) for the https://github.com/eb4x/pm3 project. PM3000 would not exist without it.

Thanks to https://github.com/aminosbh/sdl2-image-sample for the SDL2 scaffold to start the project.

### Uses
- https://github.com/btzy/nativefiledialog-extended
- https://github.com/viznut/unscii
- https://www.1001fonts.com/acknowledge-tt-brk-font.html
