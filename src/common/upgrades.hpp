// Upgrades bought with eddies in the shop: they apply to every runner. Kept small, and priced high on
// purpose: a reward for playing a lot, not a shortcut. The client keeps the levels in its wallet and
// sends them when it joins a run (MSG_CONNECT); the server applies them.
//
// To add one: an entry in the enum and in UPGRADES (same order), then its effect where the server uses it.
// Levels are sent and saved by position: only add at the end. Removing one changes the positions:
// the wallet then reads the old saves with a legacy order (wallet.cpp).

#ifndef UPGRADES_HPP
    #define UPGRADES_HPP

#include <array>
#include "powerups.hpp"

enum Upgrade { UPGRADE_SANDEVISTAN, UPGRADE_MAGNET, UPGRADE_SHIELD, UPGRADE_COUNT };

struct UpgradeInfo {
    const char *name;
    const char *effect;     // what one level does, as the shop shows it
    int max_level;
    int prices[3];          // eddies for levels 1, 2 and 3
};

inline constexpr UpgradeInfo UPGRADES[UPGRADE_COUNT] = {
    {"SANDEVISTAN",     "+0.5 S OF SPRINT",             3, {1000, 2500, 5000}},
    {"EDDIE MAGNET",    "PULLS EDDIES 0.5 TILE FURTHER",3, {1000, 2500, 5000}},
    {"SUBDERMAL ARMOR", "START EVERY RUN SHIELDED",     1, {5000}},
};

// A runner's level in each upgrade
using UpgradeLevels = std::array<int, UPGRADE_COUNT>;

constexpr float SANDEVISTAN_PER_LEVEL = 0.5f;
constexpr float MAGNET_PER_LEVEL = 0.5f;

inline float sandevistanTime(const UpgradeLevels &levels)
{
    return SANDEVISTAN_TIME + SANDEVISTAN_PER_LEVEL * levels[UPGRADE_SANDEVISTAN];
}

// Eddies whose center is this many tiles from the body are picked up too
inline float magnetReach(const UpgradeLevels &levels)
{
    return MAGNET_PER_LEVEL * levels[UPGRADE_MAGNET];
}

#endif
