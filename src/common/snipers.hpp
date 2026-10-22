// Arasaka snipers: a laser sight crosses the screen at a player's height and follows it, freezes,
// then a railgun shot sweeps the whole row. Shared by the server (who aims, fires and checks hits)
// and the client (who draws them).

#ifndef SNIPERS_HPP
    #define SNIPERS_HPP

#include <cstdint>

// No shot before this distance, then one every SNIPER_MIN_INTERVAL to SNIPER_MAX_INTERVAL seconds
constexpr float SNIPER_FIRST_DISTANCE = 150;
constexpr float SNIPER_MIN_INTERVAL = 5;
constexpr float SNIPER_MAX_INTERVAL = 10;

// The sight follows its target this long, stays still this long, then the shot lasts this long
constexpr float SNIPER_TRACK_TIME = 1.2f;
constexpr float SNIPER_AIM_TIME = 0.6f;
constexpr float SNIPER_FIRE_TIME = 0.25f;

// Half the height of the railgun's beam, in tiles
constexpr float SNIPER_BEAM_HALF = 0.35f;

enum class SniperPhase : uint8_t { Tracking, Aiming, Firing };

// MSG_COLLISION type: a railgun shot hit a player
constexpr char SNIPER_HIT = 's';

// A beam along row y against a player's body, 2 tiles tall from its top py; the whole row, any x
inline bool sniperHits(float y, float py)
{
    return y - SNIPER_BEAM_HALF < py + 1.8f && y + SNIPER_BEAM_HALF > py + 0.2f;
}

#endif
