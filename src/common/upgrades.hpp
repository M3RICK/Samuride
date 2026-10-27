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
#include "shift.hpp"

enum Upgrade {
    UPGRADE_SANDEVISTAN, UPGRADE_MAGNET, UPGRADE_SHIELD, UPGRADE_TRAUMA, UPGRADE_LUCKY, UPGRADE_SERUM, UPGRADE_PINATA,
    UPGRADE_COUNT
};

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
    {"TRAUMA TEAM",     "ONE RESCUE A RUN, BILLED HALF",1, {5000}},
    {"LUCKY CHIP",      "+2% EDDIE JACKPOTS (+10)",     3, {1500, 3000, 6000}},
    {"TITAN SERUM",     "+1 S OF TITAN, EDDIES X2",     3, {1000, 2500, 5000}},
    {"DRONE PINATA",    "+15% DRONE DROPS",             3, {1000, 2500, 5000}},
};

// A runner's level in each upgrade
using UpgradeLevels = std::array<int, UPGRADE_COUNT>;

inline float SANDEVISTAN_PER_LEVEL = 0.5f;
inline float MAGNET_PER_LEVEL = 0.5f;
// Trauma Team: a dead runner picked up once per run, back in with some grace, the run's eddies cut by the bill
inline float TRAUMA_GRACE = 2.0f;
inline float TRAUMA_BILL = 0.5f;
// Lucky Chip: each eddy may be a jackpot worth this many
inline float JACKPOT_PER_LEVEL = 0.02f;
inline int JACKPOT_EDDIES = 10;
// Titan Serum: the titan lasts longer, and from the first level its eddies count double
inline float SERUM_PER_LEVEL = 1.0f;
// Drone Pinata (CYBERPSYCHO): a drone that would drop nothing may drop a power-up
inline float PINATA_PER_LEVEL = 0.15f;

// MSG_COLLISION types: the Trauma Team picked a player up; an eddy hit the jackpot
constexpr char TRAUMA_RESCUE = 'T';
constexpr char JACKPOT = 'J';

inline float sandevistanTime(const UpgradeLevels &levels)
{
    return SANDEVISTAN_TIME + SANDEVISTAN_PER_LEVEL * levels[UPGRADE_SANDEVISTAN];
}

// Eddies whose center is this many tiles from the body are picked up too
inline float magnetReach(const UpgradeLevels &levels)
{
    return MAGNET_PER_LEVEL * levels[UPGRADE_MAGNET];
}

inline float titanTime(const UpgradeLevels &levels)
{
    return TITAN_TIME + SERUM_PER_LEVEL * levels[UPGRADE_SERUM];
}

inline float jackpotChance(const UpgradeLevels &levels)
{
    return JACKPOT_PER_LEVEL * levels[UPGRADE_LUCKY];
}

inline float pinataChance(const UpgradeLevels &levels)
{
    return PINATA_PER_LEVEL * levels[UPGRADE_PINATA];
}

#endif
