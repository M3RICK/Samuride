// Missiles fly in from the right at a player's height: a warning on the right edge follows them,
// then locks, and they keep that line. Shared by the server (who moves them and checks hits)
// and the client (who draws them).

#ifndef MISSILES_HPP
    #define MISSILES_HPP

// No missile before this distance, then one every MISSILE_MIN_INTERVAL to MISSILE_MAX_INTERVAL seconds
constexpr float MISSILE_FIRST_DISTANCE = 150;
constexpr float MISSILE_MIN_INTERVAL = 5;
constexpr float MISSILE_MAX_INTERVAL = 10;

// They appear this far ahead of their target and fly left at MISSILE_SPEED, on top of the run:
// about 1.5 to 2 s off screen, warned on the right edge
constexpr float MISSILE_SPAWN_AHEAD = 55;
constexpr float MISSILE_SPEED = 12;     // tiles per second
// The warning follows the target's height this long, then the line is locked
constexpr float MISSILE_TRACK_TIME = 1.0f;

constexpr float MISSILE_RADIUS = 0.35f;

// MSG_COLLISION type: a missile hit a player
constexpr char MISSILE_HIT = 'm';

// The missile's tip (x, y) against a player's body, 1 tile wide and 2 tall from its top-left (px, py)
inline bool missileHits(float x, float y, float px, float py)
{
    return x - MISSILE_RADIUS < px + 0.8f && x + MISSILE_RADIUS > px + 0.2f &&
           y - MISSILE_RADIUS < py + 1.8f && y + MISSILE_RADIUS > py + 0.2f;
}

#endif
