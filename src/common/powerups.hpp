// Power-ups besides the Attack Titan, shared by the server (who decides) and the client (who draws).
// They float in the map now and then, and some of the drones you shoot down drop one.

#ifndef POWERUPS_HPP
    #define POWERUPS_HPP

// Subdermal armor: takes one deadly hit instead of you
constexpr char SHIELD_TILE = 'a';
// Sandevistan: a few seconds of sprint nothing can hurt
constexpr char SANDEVISTAN_TILE = 'v';
// Collision type: a shield took a hit
constexpr char SHIELD_BREAK = 'b';

constexpr float SHIELD_GRACE = 1.0f;        // invincible after the shield breaks, to get clear
constexpr float SANDEVISTAN_TIME = 3.0f;
constexpr float SANDEVISTAN_SPEED = 1.7f;   // times the run speed

// Map chunks are 40 tiles: none in the first ~120 m, then this chance per chunk
constexpr int POWERUP_FIRST_CHUNK = 3;
constexpr float POWERUP_CHANCE = 0.2f;

inline bool isPowerUp(char tile)
{
    return tile == SHIELD_TILE || tile == SANDEVISTAN_TILE;
}

// One destroyed drone in three leaves a power-up behind ('_' for none). Picked from its
// position, so the server and the clients agree without a word on the network.
inline char droneDrop(int x, int y)
{
    unsigned hash = static_cast<unsigned>(x) * 2654435761u ^ static_cast<unsigned>(y) * 40503u;

    hash ^= hash >> 13;
    if (hash % 3 != 0)
        return '_';
    return (hash / 3) % 2 ? SHIELD_TILE : SANDEVISTAN_TILE;
}

#endif
