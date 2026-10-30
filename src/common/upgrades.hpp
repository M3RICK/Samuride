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
// A deadly hit asks first whether to call it: seconds the run waits for the answer, then it comes
inline float TRAUMA_OFFER_TIME = 5.0f;
// The rescue itself: the runner down, the AV flying in and the medic at work, the whole run waiting
inline float TRAUMA_RESCUE_TIME = 3.5f;
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
// ... and the Trauma Team asking a player hit for good whether to come
constexpr char TRAUMA_OFFER = 'O';
constexpr char JACKPOT = 'J';

// The Ripperdoc, once Smasher's arena is behind (CYBERPSYCHO): the run stops RIPPERDOC_TIME seconds and each
// runner may spend the run's eddies on one implant, for the rest of the run. Implant 0 is none.
constexpr int IMPLANTS = 3;
struct ImplantLook {
    const char *name;
    const char *what;
};
inline constexpr ImplantLook IMPLANT_LOOKS[IMPLANTS + 1] = {
    {"", ""},
    {"SUBDERMAL ARMOR", "A shield, back 30 s after it breaks"},
    {"KERENZIKOV", "Attacks 30% faster"},
    {"SYNAPTIC ACCELERATOR", "Ultimate charges twice as fast"},
};
inline int IMPLANT_PRICES[IMPLANTS + 1] = {0, 300, 150, 200};
inline float RIPPERDOC_TIME = 10;
inline float ARMOR_REGEN = 30;          // Subdermal Armor: seconds before a broken shield grows back
inline float KERENZIKOV_RATE = 1.3f;    // attack cooldowns run that much faster
inline int SYNAPTIC_RATE = 2;           // the ultimate gauge fills that much faster
// MSG_COLLISION types: the Ripperdoc opens for a player; they bought implant x
constexpr char RIPPERDOC_OFFER = 'R';
constexpr char IMPLANT_INSTALLED = 'I';

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
