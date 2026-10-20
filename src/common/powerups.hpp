// Power-ups besides the Attack Titan, shared by the server (who decides) and the client (who draws).
// They float in the map now and then, and some of the drones you shoot down drop one.

#ifndef POWERUPS_HPP
    #define POWERUPS_HPP

#include <algorithm>
#include <iterator>
#include "map.hpp"

// Subdermal armor: takes one deadly hit instead of you
constexpr char SHIELD_TILE = 'a';
// Sandevistan: a few seconds of sprint nothing can hurt
constexpr char SANDEVISTAN_TILE = 'v';
// Berserk (CYBERPSYCHO only): a few seconds of attacking on your own, twice as often, Lucy's RAM coming back faster
constexpr char BERSERK_TILE = 'r';
// Collision type: a shield took a hit
constexpr char SHIELD_BREAK = 'b';

constexpr float SHIELD_GRACE = 1.0f;        // invincible after the shield breaks, to get clear
constexpr float END_GRACE = 1.0f;           // and after the Sandevistan or the titan wears off
constexpr float SANDEVISTAN_TIME = 3.0f;
constexpr float SANDEVISTAN_SPEED = 1.7f;   // times the run speed
constexpr float BERSERK_TIME = 8.0f;
constexpr float BERSERK_ATTACK_RATE = 2.0f; // attack cooldowns run out this many times faster
constexpr float BERSERK_RAM_RATE = 3.0f;    // and Lucy's RAM comes back this many times faster

// Map chunks are 40 tiles: none in the first ~120 m, then this chance per chunk
constexpr int POWERUP_FIRST_CHUNK = 3;
constexpr float POWERUP_CHANCE = 0.2f;

// Every power-up but the Attack Titan's orb: a new one goes here too
inline constexpr char POWER_UPS[] = {SHIELD_TILE, SANDEVISTAN_TILE, BERSERK_TILE};

inline bool isPowerUp(char tile)
{
    return std::find(std::begin(POWER_UPS), std::end(POWER_UPS), tile) != std::end(POWER_UPS);
}

// One destroyed drone in three leaves a power-up behind (EMPTY_TILE for none), any of them. Picked from
// its position, so the server and the clients agree without a word on the network.
inline char droneDrop(int x, int y)
{
    unsigned hash = static_cast<unsigned>(x) * 2654435761u ^ static_cast<unsigned>(y) * 40503u;

    hash ^= hash >> 13;
    if (hash % 3 != 0)
        return EMPTY_TILE;
    return POWER_UPS[(hash / 3) % std::size(POWER_UPS)];
}

#endif
