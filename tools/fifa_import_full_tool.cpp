// Command-line helper to import team/player CSV data into PM3 with full club control.
// This tool accepts separate clubs and players CSV files to import all 244 teams.
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>
#include <array>
#include <random>

#include "io.h"
#include "pm3_defs.hh"

namespace {

struct Args {
    std::string clubsCsvFile;
    std::string playersCsvFile;
    std::string pm3Path;
    int gameNumber = 0;
    int year = 0;
    bool verbose = false;
    bool baseData = false;
    bool importLoans = false;
    int maxPlayersPerClub = 16;
};

std::optional<Args> parseArgs(int argc, char **argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--clubs" || a == "-c") && i + 1 < argc) {
            args.clubsCsvFile = argv[++i];
        } else if ((a == "--players" || a == "-p") && i + 1 < argc) {
            args.playersCsvFile = argv[++i];
        } else if ((a == "--pm3") && i + 1 < argc) {
            args.pm3Path = argv[++i];
        } else if ((a == "--game" || a == "-g") && i + 1 < argc) {
            args.gameNumber = std::atoi(argv[++i]);
        } else if ((a == "--year" || a == "-y") && i + 1 < argc) {
            args.year = std::atoi(argv[++i]);
        } else if (a == "--verbose" || a == "-v") {
            args.verbose = true;
        } else if (a == "--base" || a == "--default") {
            args.baseData = true;
        } else if (a == "--import-loans") {
            args.importLoans = true;
        } else if (a == "--max-players" && i + 1 < argc) {
            args.maxPlayersPerClub = std::atoi(argv[++i]);
        }
    }

    if (args.clubsCsvFile.empty() || args.playersCsvFile.empty() || args.pm3Path.empty()) {
        return std::nullopt;
    }
    if (!args.baseData && (args.gameNumber < 1 || args.gameNumber > 8)) {
        return std::nullopt;
    }
    if (args.maxPlayersPerClub < 1 || args.maxPlayersPerClub > 24) {
        args.maxPlayersPerClub = 16;
    }
    return args;
}

// CSV parsing utilities
std::vector<std::string> splitCsv(const std::string &line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }
    fields.push_back(current);
    return fields;
}

int parseNumber(const std::string &field) {
    std::string digits;
    digits.reserve(field.size());
    for (char c : field) {
        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && digits.empty())) {
            digits.push_back(c);
        } else if (!digits.empty()) {
            break;
        }
    }
    if (digits.empty() || digits == "-") {
        return 0;
    }
    try {
        return std::stoi(digits);
    } catch (...) {
        return 0;
    }
}

uint8_t clampByte(int v) {
    if (v < 0) return 0;
    if (v > 99) return 99;
    return static_cast<uint8_t>(v);
}

uint8_t clampNibble(int v) {
    if (v < 0) return 0;
    if (v > 15) return 15;
    return static_cast<uint8_t>(v);
}

std::string sanitizeName(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    bool lastSpace = false;
    for (unsigned char c : raw) {
        if (c >= 32 && c < 127) {
            if (std::isspace(c)) {
                if (!lastSpace) {
                    out.push_back(' ');
                }
                lastSpace = true;
            } else {
                out.push_back(static_cast<char>(c));
                lastSpace = false;
            }
        } else {
            out.push_back('?');
            lastSpace = false;
        }
    }
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) {
        out.pop_back();
    }
    return out;
}

std::string trimCopy(const std::string &raw) {
    size_t start = 0;
    while (start < raw.size() && std::isspace(static_cast<unsigned char>(raw[start]))) {
        ++start;
    }
    size_t end = raw.size();
    while (end > start && std::isspace(static_cast<unsigned char>(raw[end - 1]))) {
        --end;
    }
    return raw.substr(start, end - start);
}

bool isLoanFieldSet(const std::string &value) {
    std::string trimmed = trimCopy(value);
    if (trimmed.empty()) {
        return false;
    }
    std::string upper = trimmed;
    for (char &c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return upper != "0" && upper != "NONE" && upper != "NULL" && upper != "N/A" && upper != "NA";
}

void writeFixed(char *dest, size_t destSize, const std::string &value) {
    std::memset(dest, ' ', destSize);
    size_t copyLen = std::min(value.size(), destSize);
    if (copyLen > 0) {
        std::memcpy(dest, value.data(), copyLen);
    }
}

void copyString(char *dest, size_t destSize, const std::string &src) {
    if (destSize == 0) return;
    std::memset(dest, ' ', destSize);
    size_t copyLen = std::min(src.size(), destSize);
    std::memcpy(dest, src.data(), copyLen);
}

// Column map for CSV parsing
struct ColumnMap {
    std::unordered_map<std::string, size_t> index;
};

ColumnMap buildColumnMap(const std::vector<std::string> &headers) {
    ColumnMap map;
    for (size_t i = 0; i < headers.size(); ++i) {
        map.index[headers[i]] = i;
    }
    return map;
}

const std::string &getField(const ColumnMap &map, const std::vector<std::string> &row, const std::string &key) {
    static const std::string kEmpty;
    auto it = map.index.find(key);
    if (it == map.index.end()) {
        return kEmpty;
    }
    size_t idx = it->second;
    if (idx >= row.size()) {
        return kEmpty;
    }
    return row[idx];
}

// Club data structures
struct ClubData {
    int clubId;
    std::string name;
    std::string manager;
    std::string stadium;
    int league; // 0-4
    int maxPlayers; // Max players to import for this club (1-24)

    struct Kit {
        uint8_t design;
        uint8_t shirt_primary_r, shirt_primary_g, shirt_primary_b;
        uint8_t shirt_secondary_r, shirt_secondary_g, shirt_secondary_b;
        uint8_t shorts_r, shorts_g, shorts_b;
        uint8_t socks_r, socks_g, socks_b;
    };

    Kit homeKit;
    Kit away1Kit;
    Kit away2Kit;
    bool hasAway1 = false;
    bool hasAway2 = false;
};

// Player data structures
struct PlayerData {
    int playerId;
    int clubId;
    std::string name;
    int age;
    std::string positions;
    int overall;
    std::string preferredFoot;

    // Stats
    int pace, shooting, passing, dribbling, defending, physic, heading;

    // GK stats
    int gkDiving, gkHandling, gkKicking, gkPositioning, gkReflexes, gkSpeed;

    // Detailed stats (optional)
    int attackingCrossing, attackingFinishing, attackingShortPassing, attackingVolleys;
    int skillDribbling, skillBallControl, skillCurve, skillFkAccuracy, skillLongPassing;
    int movementAcceleration, movementSprintSpeed, movementAgility, movementReactions, movementBalance;
    int powerShotPower, powerJumping, powerStamina, powerStrength, powerLongShots;
    int mentalityInterceptions, mentalityPositioning, mentalityVision, mentalityPenalties;
    int mentalityAggression, mentalityComposure;
    int defendingMarking, defendingStandingTackle, defendingSlidingTackle;

    // Contract
    int contractYear;
    std::string loanedFrom;
};

// Parse club CSV row
std::optional<ClubData> parseClubRow(const ColumnMap &cols, const std::vector<std::string> &row, int defaultMaxPlayers) {
    ClubData club;
    club.clubId = parseNumber(getField(cols, row, "club_id"));
    club.name = sanitizeName(getField(cols, row, "club_name"));
    club.manager = sanitizeName(getField(cols, row, "manager_name"));
    club.stadium = sanitizeName(getField(cols, row, "stadium_name"));
    club.league = parseNumber(getField(cols, row, "league"));

    // Read max_players column, or use default if not provided
    int maxPlayersValue = parseNumber(getField(cols, row, "max_players"));
    club.maxPlayers = (maxPlayersValue > 0) ? std::clamp(maxPlayersValue, 1, 24) : defaultMaxPlayers;

    if (club.name.empty() || club.clubId == 0) {
        return std::nullopt;
    }

    // Parse home kit
    club.homeKit.design = clampByte(parseNumber(getField(cols, row, "home_shirt_design")));
    club.homeKit.shirt_primary_r = clampNibble(parseNumber(getField(cols, row, "home_shirt_primary_r")));
    club.homeKit.shirt_primary_g = clampNibble(parseNumber(getField(cols, row, "home_shirt_primary_g")));
    club.homeKit.shirt_primary_b = clampNibble(parseNumber(getField(cols, row, "home_shirt_primary_b")));
    club.homeKit.shirt_secondary_r = clampNibble(parseNumber(getField(cols, row, "home_shirt_secondary_r")));
    club.homeKit.shirt_secondary_g = clampNibble(parseNumber(getField(cols, row, "home_shirt_secondary_g")));
    club.homeKit.shirt_secondary_b = clampNibble(parseNumber(getField(cols, row, "home_shirt_secondary_b")));
    club.homeKit.shorts_r = clampNibble(parseNumber(getField(cols, row, "home_shorts_r")));
    club.homeKit.shorts_g = clampNibble(parseNumber(getField(cols, row, "home_shorts_g")));
    club.homeKit.shorts_b = clampNibble(parseNumber(getField(cols, row, "home_shorts_b")));
    club.homeKit.socks_r = clampNibble(parseNumber(getField(cols, row, "home_socks_r")));
    club.homeKit.socks_g = clampNibble(parseNumber(getField(cols, row, "home_socks_g")));
    club.homeKit.socks_b = clampNibble(parseNumber(getField(cols, row, "home_socks_b")));

    // Parse away kits if present
    if (cols.index.count("away1_shirt_design")) {
        club.away1Kit.design = clampByte(parseNumber(getField(cols, row, "away1_shirt_design")));
        club.away1Kit.shirt_primary_r = clampNibble(parseNumber(getField(cols, row, "away1_shirt_primary_r")));
        club.away1Kit.shirt_primary_g = clampNibble(parseNumber(getField(cols, row, "away1_shirt_primary_g")));
        club.away1Kit.shirt_primary_b = clampNibble(parseNumber(getField(cols, row, "away1_shirt_primary_b")));
        club.away1Kit.shirt_secondary_r = clampNibble(parseNumber(getField(cols, row, "away1_shirt_secondary_r")));
        club.away1Kit.shirt_secondary_g = clampNibble(parseNumber(getField(cols, row, "away1_shirt_secondary_g")));
        club.away1Kit.shirt_secondary_b = clampNibble(parseNumber(getField(cols, row, "away1_shirt_secondary_b")));
        club.away1Kit.shorts_r = clampNibble(parseNumber(getField(cols, row, "away1_shorts_r")));
        club.away1Kit.shorts_g = clampNibble(parseNumber(getField(cols, row, "away1_shorts_g")));
        club.away1Kit.shorts_b = clampNibble(parseNumber(getField(cols, row, "away1_shorts_b")));
        club.away1Kit.socks_r = clampNibble(parseNumber(getField(cols, row, "away1_socks_r")));
        club.away1Kit.socks_g = clampNibble(parseNumber(getField(cols, row, "away1_socks_g")));
        club.away1Kit.socks_b = clampNibble(parseNumber(getField(cols, row, "away1_socks_b")));
        club.hasAway1 = true;
    }

    if (cols.index.count("away2_shirt_design")) {
        club.away2Kit.design = clampByte(parseNumber(getField(cols, row, "away2_shirt_design")));
        club.away2Kit.shirt_primary_r = clampNibble(parseNumber(getField(cols, row, "away2_shirt_primary_r")));
        club.away2Kit.shirt_primary_g = clampNibble(parseNumber(getField(cols, row, "away2_shirt_primary_g")));
        club.away2Kit.shirt_primary_b = clampNibble(parseNumber(getField(cols, row, "away2_shirt_primary_b")));
        club.away2Kit.shirt_secondary_r = clampNibble(parseNumber(getField(cols, row, "away2_shirt_secondary_r")));
        club.away2Kit.shirt_secondary_g = clampNibble(parseNumber(getField(cols, row, "away2_shirt_secondary_g")));
        club.away2Kit.shirt_secondary_b = clampNibble(parseNumber(getField(cols, row, "away2_shirt_secondary_b")));
        club.away2Kit.shorts_r = clampNibble(parseNumber(getField(cols, row, "away2_shorts_r")));
        club.away2Kit.shorts_g = clampNibble(parseNumber(getField(cols, row, "away2_shorts_g")));
        club.away2Kit.shorts_b = clampNibble(parseNumber(getField(cols, row, "away2_shorts_b")));
        club.away2Kit.socks_r = clampNibble(parseNumber(getField(cols, row, "away2_socks_r")));
        club.away2Kit.socks_g = clampNibble(parseNumber(getField(cols, row, "away2_socks_g")));
        club.away2Kit.socks_b = clampNibble(parseNumber(getField(cols, row, "away2_socks_b")));
        club.hasAway2 = true;
    }

    return club;
}

int avgOrZero(std::initializer_list<int> values) {
    int sum = 0;
    int count = 0;
    for (int v : values) {
        if (v > 0) {
            sum += v;
            ++count;
        }
    }
    if (count == 0) {
        return 0;
    }
    return sum / count;
}

// Parse player CSV row
std::optional<PlayerData> parsePlayerRow(const ColumnMap &cols, const std::vector<std::string> &row) {
    PlayerData player;
    player.playerId = parseNumber(getField(cols, row, "player_id"));
    player.clubId = parseNumber(getField(cols, row, "club_id"));
    player.name = sanitizeName(getField(cols, row, "short_name"));
    player.age = parseNumber(getField(cols, row, "age"));
    player.positions = getField(cols, row, "player_positions");
    player.overall = parseNumber(getField(cols, row, "overall"));
    player.preferredFoot = getField(cols, row, "preferred_foot");

    if (player.name.empty() || player.clubId == 0 || player.overall <= 0) {
        return std::nullopt;
    }

    // Main stats
    player.pace = parseNumber(getField(cols, row, "pace"));
    player.shooting = parseNumber(getField(cols, row, "shooting"));
    player.passing = parseNumber(getField(cols, row, "passing"));
    player.dribbling = parseNumber(getField(cols, row, "dribbling"));
    player.defending = parseNumber(getField(cols, row, "defending"));
    player.physic = parseNumber(getField(cols, row, "physic"));
    player.heading = parseNumber(getField(cols, row, "attacking_heading_accuracy"));

    // GK stats
    player.gkDiving = parseNumber(getField(cols, row, "goalkeeping_diving"));
    player.gkHandling = parseNumber(getField(cols, row, "goalkeeping_handling"));
    player.gkKicking = parseNumber(getField(cols, row, "goalkeeping_kicking"));
    player.gkPositioning = parseNumber(getField(cols, row, "goalkeeping_positioning"));
    player.gkReflexes = parseNumber(getField(cols, row, "goalkeeping_reflexes"));
    player.gkSpeed = parseNumber(getField(cols, row, "goalkeeping_speed"));

    // Detailed stats
    player.attackingCrossing = parseNumber(getField(cols, row, "attacking_crossing"));
    player.attackingFinishing = parseNumber(getField(cols, row, "attacking_finishing"));
    player.attackingShortPassing = parseNumber(getField(cols, row, "attacking_short_passing"));
    player.attackingVolleys = parseNumber(getField(cols, row, "attacking_volleys"));
    player.skillDribbling = parseNumber(getField(cols, row, "skill_dribbling"));
    player.skillBallControl = parseNumber(getField(cols, row, "skill_ball_control"));
    player.skillCurve = parseNumber(getField(cols, row, "skill_curve"));
    player.skillFkAccuracy = parseNumber(getField(cols, row, "skill_fk_accuracy"));
    player.skillLongPassing = parseNumber(getField(cols, row, "skill_long_passing"));
    player.movementAcceleration = parseNumber(getField(cols, row, "movement_acceleration"));
    player.movementSprintSpeed = parseNumber(getField(cols, row, "movement_sprint_speed"));
    player.movementAgility = parseNumber(getField(cols, row, "movement_agility"));
    player.movementReactions = parseNumber(getField(cols, row, "movement_reactions"));
    player.movementBalance = parseNumber(getField(cols, row, "movement_balance"));
    player.powerShotPower = parseNumber(getField(cols, row, "power_shot_power"));
    player.powerJumping = parseNumber(getField(cols, row, "power_jumping"));
    player.powerStamina = parseNumber(getField(cols, row, "power_stamina"));
    player.powerStrength = parseNumber(getField(cols, row, "power_strength"));
    player.powerLongShots = parseNumber(getField(cols, row, "power_long_shots"));
    player.mentalityInterceptions = parseNumber(getField(cols, row, "mentality_interceptions"));
    player.mentalityPositioning = parseNumber(getField(cols, row, "mentality_positioning"));
    player.mentalityVision = parseNumber(getField(cols, row, "mentality_vision"));
    player.mentalityPenalties = parseNumber(getField(cols, row, "mentality_penalties"));
    player.mentalityAggression = parseNumber(getField(cols, row, "mentality_aggression"));
    player.mentalityComposure = parseNumber(getField(cols, row, "mentality_composure"));
    player.defendingMarking = parseNumber(getField(cols, row, "defending_marking_awareness"));
    player.defendingStandingTackle = parseNumber(getField(cols, row, "defending_standing_tackle"));
    player.defendingSlidingTackle = parseNumber(getField(cols, row, "defending_sliding_tackle"));

    // Contract
    player.contractYear = parseNumber(getField(cols, row, "club_contract_valid_until_year"));
    player.loanedFrom = getField(cols, row, "club_loaned_from");

    return player;
}

bool hasBothFootedPositions(std::string_view positions) {
    auto contains = [&](std::string_view token) {
        return positions.find(token) != std::string_view::npos;
    };
    return (contains("LB") && contains("RB")) ||
           (contains("LM") && contains("RM")) ||
           (contains("LW") && contains("RW"));
}

uint8_t scaleToTen(int stat) {
    int scaled = (stat + 5) / 10;
    if (scaled < 0) return 0;
    if (scaled > 9) return 9;
    return static_cast<uint8_t>(scaled);
}

void packPeriodTypeAndContract(PlayerRecord &rec, uint8_t periodType, uint8_t contract) {
    uint8_t packed = static_cast<uint8_t>(((contract & 0x7) << 5) | (periodType & 0x1F));
    auto *bytes = reinterpret_cast<uint8_t *>(&rec);
    bytes[offsetof(PlayerRecord, period) + 1] = packed;
}

// Build PM3 player record from parsed data
PlayerRecord buildPlayerRecord(const PlayerData &player, int baseYear, bool onLoan) {
    PlayerRecord rec{};
    writeFixed(rec.name, sizeof(rec.name), player.name);

    uint8_t overall = clampByte(player.overall);
    rec.u13 = rec.u15 = rec.u17 = rec.u19 = rec.u21 = rec.u23 = rec.u25 = overall;

    // GK handling
    int gkSum = player.gkDiving + player.gkHandling + player.gkKicking +
                player.gkPositioning + player.gkReflexes + player.gkSpeed;
    rec.hn = clampByte(gkSum / 6);

    // Calculate composite stats if detailed stats are available
    int shooting = player.shooting > 0 ? player.shooting
                   : avgOrZero({player.attackingFinishing, player.attackingVolleys,
                                player.powerShotPower, player.powerLongShots,
                                player.mentalityPositioning, player.mentalityPenalties});
    int passing = player.passing > 0 ? player.passing
                  : avgOrZero({player.attackingCrossing, player.attackingShortPassing,
                               player.skillCurve, player.skillLongPassing, player.mentalityVision});
    int dribbling = player.dribbling > 0 ? player.dribbling
                    : avgOrZero({player.skillDribbling, player.skillBallControl,
                                 player.movementAgility, player.movementBalance, player.movementReactions});
    int defending = player.defending > 0 ? player.defending
                    : avgOrZero({player.defendingMarking, player.defendingStandingTackle,
                                 player.defendingSlidingTackle, player.mentalityInterceptions});

    rec.tk = clampByte(defending);
    rec.ps = clampByte(passing);
    rec.sh = clampByte(shooting);
    rec.hd = clampByte(player.heading);
    rec.cr = clampByte(dribbling);
    rec.ft = clampByte(player.physic > 0 ? player.physic : 85);

    rec.morl = 0;
    rec.aggr = std::min<uint8_t>(scaleToTen(player.mentalityAggression), 9);

    rec.ins = 0;
    rec.age = static_cast<uint8_t>(std::clamp(player.age, 16, 34));

    int foot = 3; // Any
    if (player.preferredFoot == "Left") foot = 0;
    else if (player.preferredFoot == "Right") foot = 1;
    else if (player.preferredFoot == "Both") foot = 2;

    if (hasBothFootedPositions(player.positions)) {
        foot = 2;
    }
    rec.foot = static_cast<uint8_t>(foot);
    rec.dpts = 0;

    rec.played = 0;
    rec.scored = 0;
    rec.unk2 = 0;

    rec.wage = 0;
    rec.ins_cost = 0;

    uint8_t period = 0;
    uint8_t periodType = 0;
    if (onLoan) {
        constexpr uint8_t kOnLoan = 20;
        constexpr int kLoanWeeks = 36;
        constexpr int kTurnsPerWeek = 3;
        int loanPeriod = kLoanWeeks * kTurnsPerWeek;
        period = static_cast<uint8_t>(std::clamp(loanPeriod, 0, 255));
        periodType = kOnLoan;
    }

    int contractYearsLeft = 0;
    if (player.contractYear > 0 && baseYear > 0) {
        contractYearsLeft = player.contractYear - baseYear;
    }
    if (contractYearsLeft <= 0) {
        contractYearsLeft = 3;
    }
    rec.contract = static_cast<uint8_t>(std::clamp(contractYearsLeft, 0, 7));
    rec.unk5 = 0;
    rec.period = period;
    rec.period_type = periodType;
    packPeriodTypeAndContract(rec, periodType, rec.contract);

    rec.train = 0;
    rec.intense = 0;

    return rec;
}

void applyKitToClub(ClubRecord &club, int kitIndex, const ClubData::Kit &kit) {
    if (kitIndex < 0 || kitIndex > 2) return;

    auto &pm3kit = club.kit[kitIndex];
    pm3kit.shirt_design = kit.design;
    pm3kit.shirt_primary_color_r = kit.shirt_primary_r;
    pm3kit.shirt_primary_color_g = kit.shirt_primary_g;
    pm3kit.shirt_primary_color_b = kit.shirt_primary_b;
    pm3kit.shirt_secondary_color_r = kit.shirt_secondary_r;
    pm3kit.shirt_secondary_color_g = kit.shirt_secondary_g;
    pm3kit.shirt_secondary_color_b = kit.shirt_secondary_b;
    pm3kit.shorts_color_r = kit.shorts_r;
    pm3kit.shorts_color_g = kit.shorts_g;
    pm3kit.shorts_color_b = kit.shorts_b;
    pm3kit.socks_color_r = kit.socks_r;
    pm3kit.socks_color_g = kit.socks_g;
    pm3kit.socks_color_b = kit.socks_b;
}

int16_t encodePlayerIndex(int16_t raw, bool swapEndian) {
    if (!swapEndian) {
        return raw;
    }
    uint16_t u = static_cast<uint16_t>(raw);
    return static_cast<int16_t>((u >> 8) | (u << 8));
}

struct ImportStats {
    int clubsImported = 0;
    int playersImported = 0;
    int playersSkipped = 0;
};

// Main import function
ImportStats importTeamData(const std::string &clubsCsvPath, const std::string &playersCsvPath,
                           int baseYear, bool verbose, bool importLoans, int defaultMaxPlayersPerClub,
                           gamea &gameDataOut, gameb &clubDataOut, gamec &playerDataOut) {
    ImportStats stats;

    // Read clubs CSV
    std::ifstream clubsIn(clubsCsvPath);
    if (!clubsIn) {
        throw std::runtime_error("Failed to open clubs CSV file: " + clubsCsvPath);
    }

    std::string clubsHeaderLine;
    if (!std::getline(clubsIn, clubsHeaderLine)) {
        throw std::runtime_error("Clubs CSV file is empty: " + clubsCsvPath);
    }

    auto clubsHeaders = splitCsv(clubsHeaderLine);
    ColumnMap clubsColMap = buildColumnMap(clubsHeaders);

    std::map<int, ClubData> clubs; // clubId -> ClubData
    std::vector<int> clubOrder; // preserve CSV order

    std::string line;
    while (std::getline(clubsIn, line)) {
        if (line.empty()) continue;
        auto fields = splitCsv(line);
        auto club = parseClubRow(clubsColMap, fields, defaultMaxPlayersPerClub);
        if (!club) continue;

        clubs[club->clubId] = *club;
        clubOrder.push_back(club->clubId);
    }
    clubsIn.close();

    if (clubs.empty()) {
        throw std::runtime_error("No valid clubs found in: " + clubsCsvPath);
    }

    if (verbose) {
        std::cout << "Loaded " << clubs.size() << " clubs from CSV\n";
    }

    // Read players CSV
    std::ifstream playersIn(playersCsvPath);
    if (!playersIn) {
        throw std::runtime_error("Failed to open players CSV file: " + playersCsvPath);
    }

    std::string playersHeaderLine;
    if (!std::getline(playersIn, playersHeaderLine)) {
        throw std::runtime_error("Players CSV file is empty: " + playersCsvPath);
    }

    auto playersHeaders = splitCsv(playersHeaderLine);
    ColumnMap playersColMap = buildColumnMap(playersHeaders);

    // Group players by club
    std::map<int, std::vector<PlayerData>> clubPlayers;

    while (std::getline(playersIn, line)) {
        if (line.empty()) continue;
        auto fields = splitCsv(line);
        auto player = parsePlayerRow(playersColMap, fields);
        if (!player) {
            ++stats.playersSkipped;
            continue;
        }

        clubPlayers[player->clubId].push_back(*player);
    }
    playersIn.close();

    if (verbose) {
        std::cout << "Loaded players for " << clubPlayers.size() << " clubs\n";
    }

    // Sort players by overall rating within each club
    for (auto &[clubId, players] : clubPlayers) {
        // Separate goalkeepers and outfield
        std::vector<PlayerData> goalkeepers;
        std::vector<PlayerData> outfield;

        for (const auto &p : players) {
            if (p.positions.find("GK") != std::string::npos) {
                goalkeepers.push_back(p);
            } else {
                outfield.push_back(p);
            }
        }

        // Sort by overall rating (highest first)
        std::sort(goalkeepers.begin(), goalkeepers.end(), [](const PlayerData &a, const PlayerData &b) {
            return a.overall > b.overall;
        });
        std::sort(outfield.begin(), outfield.end(), [](const PlayerData &a, const PlayerData &b) {
            return a.overall > b.overall;
        });

        // Rebuild sorted list: goalkeepers first, then outfield
        players.clear();
        players.insert(players.end(), goalkeepers.begin(), goalkeepers.end());
        players.insert(players.end(), outfield.begin(), outfield.end());
    }

    // Initialize player pool
    constexpr size_t kPlayerCapacity = 3932;
    size_t nextPlayerIdx = 0;
    std::vector<bool> usedPlayerSlots(kPlayerCapacity, false);

    auto allocatePlayerSlot = [&]() -> int {
        while (nextPlayerIdx < kPlayerCapacity && usedPlayerSlots[nextPlayerIdx]) {
            ++nextPlayerIdx;
        }
        if (nextPlayerIdx >= kPlayerCapacity) {
            return -1;
        }
        int idx = static_cast<int>(nextPlayerIdx);
        usedPlayerSlots[nextPlayerIdx] = true;
        ++nextPlayerIdx;
        return idx;
    };

    // Import clubs into PM3
    int pm3ClubIdx = 0;
    constexpr int kMaxClubs = 244;

    for (int clubId : clubOrder) {
        if (pm3ClubIdx >= kMaxClubs) {
            if (verbose) {
                std::cout << "Reached maximum club limit (" << kMaxClubs << "), stopping import\n";
            }
            break;
        }

        auto clubIt = clubs.find(clubId);
        if (clubIt == clubs.end()) continue;

        const ClubData &clubData = clubIt->second;
        ClubRecord &pm3Club = clubDataOut.club[pm3ClubIdx];

        // Set club metadata
        copyString(pm3Club.name, sizeof(pm3Club.name), clubData.name);
        copyString(pm3Club.manager, sizeof(pm3Club.manager), clubData.manager);
        copyString(pm3Club.stadium, sizeof(pm3Club.stadium), clubData.stadium);
        pm3Club.league = static_cast<uint8_t>(std::clamp(clubData.league, 0, 4));

        // Apply kits
        applyKitToClub(pm3Club, 0, clubData.homeKit);
        if (clubData.hasAway1) {
            applyKitToClub(pm3Club, 1, clubData.away1Kit);
        } else {
            applyKitToClub(pm3Club, 1, clubData.homeKit);
        }
        if (clubData.hasAway2) {
            applyKitToClub(pm3Club, 2, clubData.away2Kit);
        } else {
            applyKitToClub(pm3Club, 2, clubData.homeKit);
        }

        // Initialize player slots
        for (int i = 0; i < 24; ++i) {
            pm3Club.player_index[i] = -1;
        }

        // Assign players
        auto playersIt = clubPlayers.find(clubId);
        if (playersIt != clubPlayers.end()) {
            const auto &players = playersIt->second;
            int assigned = 0;
            int clubMaxPlayers = clubData.maxPlayers; // Use per-club limit

            for (const auto &playerData : players) {
                if (assigned >= clubMaxPlayers) {
                    ++stats.playersSkipped;
                    continue;
                }

                int playerSlot = allocatePlayerSlot();
                if (playerSlot == -1) {
                    if (verbose) {
                        std::cout << "Player capacity exhausted\n";
                    }
                    break;
                }

                bool onLoan = importLoans && isLoanFieldSet(playerData.loanedFrom);
                PlayerRecord rec = buildPlayerRecord(playerData, baseYear, onLoan);
                playerDataOut.player[playerSlot] = rec;
                pm3Club.player_index[assigned] = encodePlayerIndex(static_cast<int16_t>(playerSlot), true);

                ++assigned;
                ++stats.playersImported;
            }

            if (verbose && assigned > 0) {
                std::cout << "  [" << pm3ClubIdx << "] " << clubData.name
                          << ": " << assigned << " players (max: " << clubMaxPlayers << ")\n";
            }
        }

        ++stats.clubsImported;
        ++pm3ClubIdx;
    }

    // Build league structure - place clubs in their specified leagues
    std::array<std::vector<int>, 5> tiers;
    for (int idx = 0; idx < pm3ClubIdx; ++idx) {
        int league = clubDataOut.club[idx].league;
        if (league >= 0 && league <= 4) {
            tiers[league].push_back(idx);
        }
    }

    // Fill league slots (max capacity per tier)
    constexpr std::array<int, 5> kStorageSizes{{22, 24, 24, 22, 22}};

    auto writeLeague = [](int16_t *dest, int storageCount, const std::vector<int> &src) {
        int fill = std::min(static_cast<int>(src.size()), storageCount);
        for (int i = 0; i < storageCount; ++i) {
            if (i < fill) {
                dest[i] = static_cast<int16_t>(src[i]);
            } else {
                dest[i] = -1;
            }
        }
    };

    writeLeague(gameDataOut.club_index.leagues.premier_league, kStorageSizes[0], tiers[0]);
    writeLeague(gameDataOut.club_index.leagues.division_one, kStorageSizes[1], tiers[1]);
    writeLeague(gameDataOut.club_index.leagues.division_two, kStorageSizes[2], tiers[2]);
    writeLeague(gameDataOut.club_index.leagues.division_three, kStorageSizes[3], tiers[3]);
    writeLeague(gameDataOut.club_index.leagues.conference_league, kStorageSizes[4], tiers[4]);

    // Set manager's club to first club
    if (pm3ClubIdx > 0) {
        gameDataOut.manager[0].club_idx = 0;
    }

    return stats;
}

} // namespace

int main(int argc, char **argv) {
    auto parsed = parseArgs(argc, argv);
    if (!parsed) {
        std::cerr << "Usage: fifa_import_full_tool --clubs <clubs.csv> --players <players.csv> --pm3 <path> "
                     "(--game <1-8> | --base) [--year <value>] [--verbose] [--import-loans] "
                     "[--max-players <N>]\n";
        return 1;
    }
    Args args = *parsed;

    if (args.baseData && args.importLoans) {
        std::cerr << "Warning: --import-loans with --base will cause loan players to show as banned "
                     "when starting a new game\n";
    }

    // Backup existing files
    if (!io::backupPm3Files(args.pm3Path)) {
        std::cerr << "Failed to backup PM3 files: " << io::pm3LastError() << "\n";
        return 1;
    }

    // Load existing data
    gamea gameDataOut{};
    gameb clubDataOut{};
    gamec playerDataOut{};

    try {
        if (args.baseData) {
            io::loadDefaultGamedata(args.pm3Path, gameDataOut);
            io::loadDefaultClubdata(args.pm3Path, clubDataOut);
            io::loadDefaultPlaydata(args.pm3Path, playerDataOut);
        } else {
            io::loadBinaries(args.gameNumber, args.pm3Path, gameDataOut, clubDataOut, playerDataOut);
        }
    } catch (const std::exception &ex) {
        std::cerr << "Failed to load PM3 data: " << ex.what() << "\n";
        return 1;
    }

    // Set year if specified
    if (args.year != 0) {
        gameDataOut.year = args.year;
    }
    int baseYear = gameDataOut.year;
    if (baseYear <= 0) {
        baseYear = 2025;
    }

    // Perform import
    try {
        ImportStats stats = importTeamData(args.clubsCsvFile, args.playersCsvFile,
                                           baseYear, args.verbose, args.importLoans, args.maxPlayersPerClub,
                                           gameDataOut, clubDataOut, playerDataOut);

        std::cout << "Import complete!\n";
        std::cout << "  Clubs imported: " << stats.clubsImported << "\n";
        std::cout << "  Players imported: " << stats.playersImported << "\n";
        std::cout << "  Players skipped: " << stats.playersSkipped << "\n";
        std::cout << "  Base year: " << baseYear << "\n";
    } catch (const std::exception &ex) {
        std::cerr << "Import failed: " << ex.what() << "\n";
        return 1;
    }

    // Save data
    try {
        if (args.baseData) {
            io::saveDefaultGamedata(args.pm3Path, gameDataOut);
            io::saveDefaultClubdata(args.pm3Path, clubDataOut);
            io::saveDefaultPlaydata(args.pm3Path, playerDataOut);
        } else {
            io::saveBinaries(args.gameNumber, args.pm3Path, gameDataOut, clubDataOut, playerDataOut);
        }
        std::cout << "Data saved successfully!\n";
    } catch (const std::exception &ex) {
        std::cerr << "Failed to save PM3 data: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
