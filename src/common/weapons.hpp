// What each runner attacks with in CYBERPSYCHO (the attack key, F by default), and how hard.
// Shared by the server (who fires and decides what is hit) and the client (who draws the shots).
// Which runner has which weapon: runners.hpp.

#ifndef WEAPONS_HPP
    #define WEAPONS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <utility>
#include "map.hpp"

enum class Weapon : uint8_t {
    Gun,        // Johnny, David: bullets
    Minigun,    // Jinx's Pow-Pow: bursts of bullets
    Blade,      // Mikasa: cuts what is right in front, titans included
    Hack,       // Lucy: switches off the first zapper or drone in her way
    Spear,      // Mikasa's thunder spears, while her ultimate lasts (ultimates.hpp)
    Rebellion,  // Dante: his sword on the ground (cuts like Blade), Ebony & Ivory in the air (shoots
                // like Gun); its shots go out as those two, it has no stats of its own
    Blink,      // Wraith: no weapon, a teleport that kills the drones it crosses (BLINK_*); no stats either
};

struct WeaponStats {
    float cooldown;     // seconds between two attacks
    float speed;        // projectile speed in tiles per second, on top of the run speed; 0 for melee
    float reach;        // how far ahead a melee strike or a hack lands, in tiles
    float radius;       // a projectile hits drones this close above or below its line, in tiles
};

// Projectiles vanish this many tiles ahead of where they were fired
inline float PROJECTILE_RANGE = 22;

// Jinx's Pow-Pow fires bursts of three; only in Berserk does it spray, one bullet at a time
inline int MINIGUN_BURST = 3;
inline float MINIGUN_BURST_GAP = 0.08f;      // seconds between the bullets of a burst
inline float MINIGUN_SPRAY_COOLDOWN = 0.2f;  // between two bullets in Berserk, before its speed-up

// Lucy's hack: the first zapper or drone in her way goes quiet, for one charge of RAM. Her way is
// HACK_RANGE tiles ahead, from HACK_LANE tiles above her head to HACK_LANE tiles below her feet.
// A hacked zapper is off for HACK_TIME, a hacked drone goes down like a shot one.
constexpr char ZAPPER_HACK = 'h';   // collision type: a zapper was hacked, at one of its nodes
// A drone she hacks passes the hack on to the nearest live drone within HACK_BOUNCE_RANGE tiles of it
constexpr char HACK_BOUNCE = 'j';   // collision type: the hack jumped to that drone, from her last kill
inline float HACK_BOUNCE_RANGE = 4;
inline float HACK_RANGE = 7;
inline int HACK_LANE = 1;
inline float HACK_TIME = 2.0f;
constexpr int RAM_MAX = 3;          // sent in 2 bits: 3 at most
inline float RAM_RECHARGE = 3.5f;    // seconds to get one charge back

// Dante's style meter: kills in a row raise his rank, from D to SSS, and each rank pays more per drone.
// STYLE_KILLS kills go up a rank; STYLE_DROP_TIME seconds without one drop a rank; a hit drops
// STYLE_HIT_DROP ranks. He also attacks faster: his cooldowns times STYLE_HASTE.
constexpr int STYLE_RANKS = 7;
inline const char *STYLE_NAMES[STYLE_RANKS] = {"D", "C", "B", "A", "S", "SS", "SSS"};
inline float STYLE_BOUNTY[STYLE_RANKS] = {1, 1, 1.5f, 2, 2, 2.5f, 3};    // times a drone's bounty
inline int STYLE_KILLS = 2;
inline float STYLE_HASTE[STYLE_RANKS] = {1, 0.95f, 0.9f, 0.85f, 0.8f, 0.7f, 0.6f};
inline float STYLE_DROP_TIME = 10;
inline int STYLE_HIT_DROP = 2;

// Wraith's Blink: BLINK_DISTANCE tiles straight ahead, at his height and keeping his vertical speed (it
// went up or down by that speed once: in the air it felt random). The drones his body crosses go down;
// he lands where the marker shows (past a zapper there, see blinkLanding). BLINK_CHARGES at most, one back every
// BLINK_RECHARGE, faster in Berserk (which doesn't blink on its own).
inline float BLINK_DISTANCE = 4;
// Held, the attack key takes him further: BLINK_FAR_DISTANCE after BLINK_HOLD_TIME, the Blink on release
inline float BLINK_FAR_DISTANCE = 8;
inline float BLINK_HOLD_TIME = 0.6f;
inline float blinkDistance(float held)
{
    return BLINK_DISTANCE + (BLINK_FAR_DISTANCE - BLINK_DISTANCE) * std::clamp(held / BLINK_HOLD_TIME, 0.0f, 1.0f);
}
inline float BLINK_COOLDOWN = 0.3f;
constexpr int BLINK_CHARGES = 3;    // sent in 2 bits: 3 at most
inline float BLINK_RECHARGE = 2;
// The runner who goes furthest: a Blink aimed into a zapper lands on the first safe column up to
// BLINK_SAFE_SEARCH tiles past it, and one through a zapper on his way pays BLINK_CLOSE_CALL_BOUNTY
constexpr int BLINK_SAFE_SEARCH = 3;
inline int BLINK_CLOSE_CALL_BOUNTY = 5;
constexpr char CLOSE_CALL = 'w';    // collision type: a Blink through danger, where he landed

// Where a Blink aimed at x lands, his top at y: the first column from there with no live zapper on his body
// (rows sampled like the collisions), x itself when none is safe. deadly(column, row) says which are live.
template <typename Deadly>
float blinkLanding(float x, float y, Deadly deadly)
{
    for (int step = 0; step <= BLINK_SAFE_SEARCH; step++) {
        int column = static_cast<int>(x + step + 0.5f);
        if (std::ranges::none_of(std::array{0.3f, 1.0f, 1.7f}, [&](float dy) { return deadly(column, static_cast<int>(y + dy)); }))
            return x + step;
    }
    return x;
}
// Blinking this close to Smasher's slam or cannon landing on him is PERFECT: he goes through it,
// untouchable until it's over, and the charge comes back
inline float BLINK_PERFECT_WINDOW = 0.3f;
constexpr char PERFECT_DODGE = 'f';     // collision type: a perfect Blink, where he landed

// In the order of Weapon: a new weapon gets its line here, and its values in tuning.cpp
inline WeaponStats WEAPON_STATS[] = {
    {0.25f, 30, 0, 0.4f},       // Gun
    {0.6f, 32, 0, 0.35f},       // Minigun: the cooldown is between two bursts
    {0.45f, 0, 2.5f, 0},        // Blade
    {0.45f, 0, 0, 0},           // Hack: its way is HACK_RANGE and HACK_LANE
    {0.45f, 28, 0, 0.6f},       // Spear
};

// A weapon from a newer version of the game fires like a gun
inline WeaponStats statsOf(Weapon weapon)
{
    auto index = static_cast<size_t>(weapon);
    return WEAPON_STATS[index < std::size(WEAPON_STATS) ? index : 0];
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
