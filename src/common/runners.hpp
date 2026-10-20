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
};

inline constexpr Runner RUNNERS[] = {
    {"johny",  0,   Weapon::Gun,     Ultimate::FireWave},
    {"david",  100, Weapon::Gun,     Ultimate::Nuke},
    {"jinx",   250, Weapon::Minigun, Ultimate::Fishbones},
    {"mikasa", 500, Weapon::Blade,   Ultimate::ThunderSpears},
    {"lucy",   750, Weapon::Hack,    Ultimate::Breach},
};

constexpr int RUNNER_COUNT = static_cast<int>(std::size(RUNNERS));

// An unknown id (from a newer version of the game) plays as the first runner
inline const Runner &runner(int id)
{
    return RUNNERS[id >= 0 && id < RUNNER_COUNT ? id : 0];
}

inline Weapon weaponOf(int id) { return runner(id).weapon; }
inline Ultimate ultimateOf(int id) { return runner(id).ultimate; }

#endif
