// What each runner attacks with in CYBERPSYCHO (the attack key, F by default), and how hard.
// Shared by the server (who fires and decides what is hit) and the client (who draws the shots).
// Which runner has which weapon: runners.hpp.

#ifndef WEAPONS_HPP
    #define WEAPONS_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include "map.hpp"

enum class Weapon : uint8_t {
    Gun,        // Johnny, David: bullets
    Minigun,    // Jinx's Pow-Pow: bursts of bullets
    Blade,      // Mikasa: cuts what is right in front, titans included
    Hack,       // Lucy: switches off the first zapper or drone in her way
    Spear,      // Mikasa's thunder spears, while her ultimate lasts (ultimates.hpp)
};

struct WeaponStats {
    float cooldown;     // seconds between two attacks
    float speed;        // projectile speed in tiles per second, on top of the run speed; 0 for melee
    float reach;        // how far ahead a melee strike or a hack lands, in tiles
    float radius;       // a projectile hits drones this close above or below its line, in tiles
};

// Projectiles vanish this many tiles ahead of where they were fired
constexpr float PROJECTILE_RANGE = 22;

// Jinx's Pow-Pow fires bursts of three; only in Berserk does it spray, one bullet at a time
constexpr int MINIGUN_BURST = 3;
constexpr float MINIGUN_BURST_GAP = 0.08f;      // seconds between the bullets of a burst
constexpr float MINIGUN_SPRAY_COOLDOWN = 0.2f;  // between two bullets in Berserk, before its speed-up

// Lucy's hack: the first zapper or drone in her way goes quiet, for one charge of RAM. Her way is
// HACK_RANGE tiles ahead, from HACK_LANE tiles above her head to HACK_LANE tiles below her feet.
// A hacked zapper is off for HACK_TIME, a hacked drone goes down like a shot one.
constexpr char ZAPPER_HACK = 'h';   // collision type: a zapper was hacked, at one of its nodes
constexpr float HACK_RANGE = 7;
constexpr int HACK_LANE = 1;
constexpr float HACK_TIME = 1.2f;
constexpr int RAM_MAX = 2;
constexpr float RAM_RECHARGE = 6.0f;    // seconds to get one charge back

inline WeaponStats statsOf(Weapon weapon)
{
    switch (weapon) {
        case Weapon::Gun:
            return {0.25f, 30, 0, 0.4f};
        case Weapon::Minigun:
            return {0.6f, 32, 0, 0.35f};    // the cooldown is between two bursts
        case Weapon::Blade:
            return {0.45f, 0, 2.5f, 0};
        case Weapon::Hack:
            return {0.45f, 0, HACK_RANGE, 0};
        case Weapon::Spear:
            return {0.45f, 28, 0, 0.6f};
    }
    return {0.25f, 30, 0, 0.4f};
}

// Where Lucy's hack lands, for a body `height` tall with its top-left corner at (x, y): the nearest
// column of her way first, then the closest to her middle. {-1, -1} when nothing is in her way.
// off(x, y): that drone is already down, or that zapper already off.
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
            bool live = (tile == DRONE_TILE || tile == ZAPPER_TILE) && !off(col, row);
            if (live && (target < 0 || std::abs(row + 0.5f - middle) < std::abs(target + 0.5f - middle)))
                target = row;
        }
        if (target >= 0)
            return {col, target};
    }
    return {-1, -1};
}

#endif
