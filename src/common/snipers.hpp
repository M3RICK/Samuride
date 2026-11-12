// Arasaka snipers: a laser sight crosses the screen at a player's height and follows it, freezes,
// then a bullet flies along the row from the right edge of the screen: leave the row, or get past the
// bullet (a Blink through it). Shared by the server (who aims, fires and checks hits) and the client
// (who draws them).

#ifndef SNIPERS_HPP
    #define SNIPERS_HPP

#include <cstdint>

// No shot before this distance, then one every SNIPER_MIN_INTERVAL to SNIPER_MAX_INTERVAL seconds, never two at once
inline float SNIPER_FIRST_DISTANCE = 150;
inline float SNIPER_MIN_INTERVAL = 5;
inline float SNIPER_MAX_INTERVAL = 10;

// The sight follows its target this long, stays still this long, then the bullet flies this long
inline float SNIPER_TRACK_TIME = 1.2f;
inline float SNIPER_AIM_TIME = 0.6f;
inline float SNIPER_FIRE_TIME = 1.2f;

// The bullet leaves this many tiles ahead of the leader (the right edge of the screen) and flies at the
// runners this fast, in tiles per second, on top of their own pace
inline float SNIPER_BULLET_AHEAD = 23;
inline float SNIPER_BULLET_SPEED = 30;
// Half its height, and its length from the nose back, in tiles
inline float SNIPER_BULLET_HALF = 0.35f;
inline float SNIPER_BULLET_LENGTH = 1.5f;

enum class SniperPhase : uint8_t { Tracking, Aiming, Firing };

// MSG_COLLISION type: a sniper's bullet hit a player
constexpr char SNIPER_HIT = 's';

// A bullet along row y against a player's body, 2 tiles tall from its top py
inline bool sniperHits(float y, float py)
{
    return y - SNIPER_BULLET_HALF < py + 1.8f && y + SNIPER_BULLET_HALF > py + 0.2f;
}

// ... its nose at x, against that body, a tile wide from its left px
inline bool sniperHits(float x, float y, float px, float py)
{
    return x < px + 1 && x + SNIPER_BULLET_LENGTH > px && sniperHits(y, py);
}

#endif
