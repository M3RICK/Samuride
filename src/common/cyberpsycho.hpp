#ifndef CYBERPSYCHO_HPP
    #define CYBERPSYCHO_HPP

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include "map.hpp"

// Rules of the CYBERPSYCHO mode, shared by the server (who decides) and the client (who draws)

enum class GameMode : uint8_t { Classic, Cyberpsycho };

// Map tile of a security drone: deadly on contact, worth DRONE_BOUNTY eddies when destroyed
constexpr char DRONE_TILE = 'd';
constexpr int DRONE_BOUNTY = 5;

// Projectiles vanish this many tiles ahead of where they were fired
constexpr float PROJECTILE_RANGE = 22;

enum class Weapon : uint8_t { Gun, Rockets, Blade, Hack };

// Lucy's hack: the first zapper or drone within HACK_RANGE tiles ahead goes quiet, for one charge of RAM.
// A hacked zapper is off for HACK_TIME, a hacked drone goes down like a shot one.
constexpr char ZAPPER_HACK = 'h';   // collision type: a zapper was hacked, at one of its nodes
constexpr float HACK_RANGE = 7;
constexpr float HACK_TIME = 1.2f;
constexpr int RAM_MAX = 2;
constexpr float RAM_RECHARGE = 6.0f;    // seconds to get one charge back

struct WeaponStats {
    float cooldown;     // seconds between two attacks
    float speed;        // projectile speed in tiles per second, on top of the run speed; 0 for melee
    float reach;        // how far ahead a melee strike lands, in tiles
    float radius;       // a projectile hits drones this close above or below its line, in tiles
};

// Character ids follow the client's CHARACTER_DIRS: johny, david, jinx, mikasa, lucy
inline Weapon weaponOf(int character)
{
    if (character == 2)
        return Weapon::Rockets;
    if (character == 3)
        return Weapon::Blade;
    if (character == 4)
        return Weapon::Hack;
    return Weapon::Gun;
}

inline WeaponStats statsOf(Weapon weapon)
{
    switch (weapon) {
        case Weapon::Rockets:
            return {0.6f, 18, 0, 0.9f};
        case Weapon::Blade:
            return {0.45f, 0, 2.5f, 0};
        case Weapon::Hack:
            return {0.45f, 0, HACK_RANGE, 0};
        default:
            return {0.25f, 30, 0, 0.4f};
    }
}

// Every node of the zapper that has one at (x, y): the nodes linked to it, the way its beams are drawn
inline std::vector<std::pair<int, int>> zapperNodes(const Map &map, int x, int y)
{
    std::vector<std::pair<int, int>> nodes = {{x, y}};

    for (size_t i = 0; i < nodes.size(); i++) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                std::pair<int, int> next = {nodes[i].first + dx, nodes[i].second + dy};
                if (next.first >= 0 && next.second >= 0 && map.getTile(next.first, next.second) == 'e' &&
                    std::find(nodes.begin(), nodes.end(), next) == nodes.end())
                    nodes.push_back(next);
            }
        }
    }
    return nodes;
}

#endif
