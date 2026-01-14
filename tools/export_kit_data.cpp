#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "pm3_defs.hh"

std::string sanitizeClubName(const char *name, size_t maxLen) {
    std::string result;
    for (size_t i = 0; i < maxLen && name[i] != '\0'; ++i) {
        if (name[i] >= 32 && name[i] <= 126) {
            result += name[i];
        }
    }
    // Trim trailing spaces
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

void exportClubKits(const gameb &clubData, bool csvFormat) {
    if (csvFormat) {
        std::cout << "club_idx,club_name,";
        std::cout << "home_design,home_primary_r,home_primary_g,home_primary_b,";
        std::cout << "home_secondary_r,home_secondary_g,home_secondary_b,";
        std::cout << "home_shorts_r,home_shorts_g,home_shorts_b,";
        std::cout << "home_socks_r,home_socks_g,home_socks_b,";
        std::cout << "away1_design,away1_primary_r,away1_primary_g,away1_primary_b,";
        std::cout << "away1_secondary_r,away1_secondary_g,away1_secondary_b,";
        std::cout << "away1_shorts_r,away1_shorts_g,away1_shorts_b,";
        std::cout << "away1_socks_r,away1_socks_g,away1_socks_b,";
        std::cout << "away2_design,away2_primary_r,away2_primary_g,away2_primary_b,";
        std::cout << "away2_secondary_r,away2_secondary_g,away2_secondary_b,";
        std::cout << "away2_shorts_r,away2_shorts_g,away2_shorts_b,";
        std::cout << "away2_socks_r,away2_socks_g,away2_socks_b\n";
    }

    for (int i = 0; i < kClubIdxMax; ++i) {
        const ClubRecord &club = clubData.club[i];
        std::string name = sanitizeClubName(club.name, sizeof(club.name));

        // Skip empty clubs
        if (name.empty() || name == "Unknown") {
            continue;
        }

        if (csvFormat) {
            std::cout << i << ",\"" << name << "\",";

            for (int kit = 0; kit < 3; ++kit) {
                std::cout << static_cast<int>(club.kit[kit].shirt_design) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_primary_color_r) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_primary_color_g) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_primary_color_b) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_secondary_color_r) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_secondary_color_g) << ",";
                std::cout << static_cast<int>(club.kit[kit].shirt_secondary_color_b) << ",";
                std::cout << static_cast<int>(club.kit[kit].shorts_color_r) << ",";
                std::cout << static_cast<int>(club.kit[kit].shorts_color_g) << ",";
                std::cout << static_cast<int>(club.kit[kit].shorts_color_b) << ",";
                std::cout << static_cast<int>(club.kit[kit].socks_color_r) << ",";
                std::cout << static_cast<int>(club.kit[kit].socks_color_g) << ",";
                std::cout << static_cast<int>(club.kit[kit].socks_color_b);
                if (kit < 2) std::cout << ",";
            }
            std::cout << "\n";
        } else {
            std::cout << "\n[" << i << "] " << name << "\n";
            const char *kitLabels[] = {"Home", "Away1", "Away2"};
            for (int kit = 0; kit < 3; ++kit) {
                std::cout << "  " << kitLabels[kit] << " Kit:\n";
                std::cout << "    Design: " << static_cast<int>(club.kit[kit].shirt_design) << "\n";
                std::cout << "    Shirt Primary RGB: ("
                         << static_cast<int>(club.kit[kit].shirt_primary_color_r) << ","
                         << static_cast<int>(club.kit[kit].shirt_primary_color_g) << ","
                         << static_cast<int>(club.kit[kit].shirt_primary_color_b) << ")\n";
                std::cout << "    Shirt Secondary RGB: ("
                         << static_cast<int>(club.kit[kit].shirt_secondary_color_r) << ","
                         << static_cast<int>(club.kit[kit].shirt_secondary_color_g) << ","
                         << static_cast<int>(club.kit[kit].shirt_secondary_color_b) << ")\n";
                std::cout << "    Shorts RGB: ("
                         << static_cast<int>(club.kit[kit].shorts_color_r) << ","
                         << static_cast<int>(club.kit[kit].shorts_color_g) << ","
                         << static_cast<int>(club.kit[kit].shorts_color_b) << ")\n";
                std::cout << "    Socks RGB: ("
                         << static_cast<int>(club.kit[kit].socks_color_r) << ","
                         << static_cast<int>(club.kit[kit].socks_color_g) << ","
                         << static_cast<int>(club.kit[kit].socks_color_b) << ")\n";
            }
        }
    }
}

int main(int argc, char **argv) {
    std::string pm3Path;
    bool csvFormat = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--pm3" && i + 1 < argc) {
            pm3Path = argv[++i];
        } else if (arg == "--csv") {
            csvFormat = true;
        }
    }

    if (pm3Path.empty()) {
        std::cerr << "Usage: export_kit_data --pm3 <path> [--csv]\n";
        std::cerr << "Export kit data from PM3 clubdata.dat\n";
        std::cerr << "\n";
        std::cerr << "Options:\n";
        std::cerr << "  --pm3 <path>   Path to PM3 installation directory\n";
        std::cerr << "  --csv          Output in CSV format\n";
        return 1;
    }

    std::filesystem::path file = std::filesystem::path(pm3Path) / std::string{kClubDataFile};
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open " << file << "\n";
        return 1;
    }

    gameb data{};
    in.read(reinterpret_cast<char *>(&data), static_cast<std::streamsize>(sizeof(gameb)));
    if (!in) {
        std::cerr << "Failed to read " << file << "\n";
        return 1;
    }

    exportClubKits(data, csvFormat);
    return 0;
}
