// The playable runners, one line each: the only list of them in the game.
//
// To add one: slice their sprite sheet into assets/<dir> with tools/slice_sheets.py, then add their
// line at the end of RUNNERS. Their position in the list is their id, sent over the network and kept
// in the saves: never reorder the list, only add to it.

#ifndef RUNNERS_HPP
    #define RUNNERS_HPP

#include <iterator>
#include "weapons.hpp"
#include "ultimates.hpp"

struct Runner {
    const char *dir;        // folder in assets/: sprite sheet, portrait and meta.txt (with the name shown)
    int price;              // eddies; they are bought in the order of the list
    Weapon weapon;          // the attack, in CYBERPSYCHO (weapons.hpp)
    Ultimate ultimate;      // fired once the gauge is full, in CYBERPSYCHO (ultimates.hpp)
    int charge;             // eddies to fill the ultimate's gauge
    float agility;          // the jetpack's lift and the fall, times this: how fast they turn in the air
};

// Prices, charges and agility can be changed from assets/tuning.txt (tuning.cpp).
// Mikasa's ODM gear: she turns faster in the air than anyone.
inline Runner RUNNERS[] = {
    {"johny",  0,   Weapon::Gun,     Ultimate::FireWave,      150, 1},
    {"david",  100, Weapon::Gun,     Ultimate::Nuke,          150, 1},
    {"jinx",   250, Weapon::Minigun, Ultimate::Fishbones,     150, 1},
    {"mikasa", 500, Weapon::Blade,   Ultimate::ThunderSpears, 100, 1.4f},
    {"lucy",   750, Weapon::Hack,    Ultimate::Breach,        150, 1},
    {"dante",  1000, Weapon::Rebellion, Ultimate::Doppelganger, 150, 1},
    {"wraith", 1250, Weapon::Blink,  Ultimate::Stillness,     150, 1},
};

constexpr int RUNNER_COUNT = static_cast<int>(std::size(RUNNERS));

// An unknown id (from a newer version of the game) plays as the first runner
inline const Runner &runner(int id)
{
    return RUNNERS[id >= 0 && id < RUNNER_COUNT ? id : 0];
}

inline Weapon weaponOf(int id) { return runner(id).weapon; }
inline Ultimate ultimateOf(int id) { return runner(id).ultimate; }
inline int ultimateCharge(int id) { return runner(id).charge; }
inline float agilityOf(int id) { return runner(id).agility; }

#endif
