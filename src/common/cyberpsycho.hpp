#ifndef CYBERPSYCHO_HPP
    #define CYBERPSYCHO_HPP

#include <algorithm>
#include <cmath>
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

enum class Weapon : uint8_t { Gun, Rockets, Blade, Hack, Spear };

// Lucy's hack: the first zapper or drone in her way goes quiet, for one charge of RAM. Her way is
// HACK_RANGE tiles ahead, from HACK_LANE tiles above her head to HACK_LANE tiles below her feet.
// A hacked zapper is off for HACK_TIME, a hacked drone goes down like a shot one.
constexpr char ZAPPER_HACK = 'h';   // collision type: a zapper was hacked, at one of its nodes
constexpr float HACK_RANGE = 7;
constexpr int HACK_LANE = 1;
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
        case Weapon::Spear:
            return {0.45f, 28, 0, 0.6f};
        default:
            return {0.25f, 30, 0, 0.4f};
    }
}

// Ultimates (CYBERPSYCHO only): every eddy you earn charges the gauge, a full one fires your runner's.
// None of them makes you invincible: they act on the scenery, you still steer and can die.
enum class Ultimate : uint8_t { None, FireWave, Nuke, Breach, ThunderSpears };

constexpr int ULTIMATE_CHARGE = 100;    // eddies to fill the gauge

// Johnny palms his gun: a wall of fire sweeps the screen ahead, burning drones, zappers and titans
constexpr float FIRE_WAVE_SPEED = 45;   // tiles per second, not on top of the run speed
constexpr float FIRE_WAVE_RANGE = 25;   // tiles from where it started

// David's arm cannon: a shell fired at the ground ahead, razing everything from floor to ceiling around
// where it lands. Measured from where it was fired: he runs into the blast, it doesn't spare him.
constexpr float NUKE_DELAY = 0.5f;      // seconds before it lands
constexpr float NUKE_DISTANCE = 10;     // tiles ahead of where it was fired
constexpr float NUKE_RADIUS = 8;        // tiles razed on each side of the impact

// Lucy's network breach: every drone on screen ahead goes down, every zapper there goes off a while
constexpr float BREACH_AHEAD = 15;      // tiles ahead of where it starts: the rest of the screen
constexpr float BREACH_TIME = 3.0f;

// Mikasa's thunder spears: her next attacks are spears, flying ahead until they stick in a drone or a
// titan (or reach their range), then blowing up in a lightning column from floor to ceiling
constexpr int THUNDER_SPEARS = 3;
constexpr float SPEAR_BLAST_RADIUS = 1;     // tiles on each side of the impact
constexpr char SPEAR_BLAST = 'l';           // collision type: a spear blew up at that column

// Character ids follow the client's CHARACTER_DIRS; runners without one yet get None
inline Ultimate ultimateOf(int character)
{
    if (character == 0)
        return Ultimate::FireWave;
    if (character == 1)
        return Ultimate::Nuke;
    if (character == 3)
        return Ultimate::ThunderSpears;
    if (character == 4)
        return Ultimate::Breach;
    return Ultimate::None;
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

// Where Lucy's hack lands, for a body `height` tall with its top-left corner at (x, y): the nearest
// column of her way first, then the closest to her middle. {-1, -1} when nothing is in her way.
// off(x, y): that drone is already down, or that zapper already hacked.
template <typename Off>
std::pair<int, int> hackTarget(const Map &map, float x, float y, float height, Off off)
{
    int front = static_cast<int>(x + 0.5f);
    int top = std::max(0, static_cast<int>(y) - HACK_LANE);
    int bottom = std::min(static_cast<int>(map.getHeight()) - 1, static_cast<int>(y + height) + HACK_LANE);
    float middle = y + height / 2;

    for (int col = front; col <= front + HACK_RANGE; col++) {
        int target = -1;
        for (int row = top; row <= bottom; row++) {
            char tile = map.getTile(col, row);
            bool live = (tile == DRONE_TILE || tile == 'e') && !off(col, row);
            if (live && (target < 0 || std::abs(row + 0.5f - middle) < std::abs(target + 0.5f - middle)))
                target = row;
        }
        if (target >= 0)
            return {col, target};
    }
    return {-1, -1};
}

#endif
