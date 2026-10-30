// Events during a run: stretches of road where something changes for EVENT_LENGTH tiles. The first at
// EVENT_FIRST (between the boons and Smasher), then one every EVENT_EVERY tiles after his arena.
// The server rolls each one's kind and tells the clients EVENT_WARNING tiles before it starts.
// Shared by the server (who rolls them, generates the swarm, flips gravity) and the client (who draws them).

#ifndef EVENTS_HPP
    #define EVENTS_HPP

#include <cstdint>
#include "boss.hpp"

enum class RunEvent : uint8_t {
    None,
    Blackout,   // the lights go out: only the zappers and drones glow
    Swarm,      // drones everywhere (CYBERPSYCHO)
    Flip,       // gravity flips: you fall up, the jetpack pushes down
};
constexpr int RUN_EVENTS = 4;

inline float EVENT_FIRST = 300;
inline float EVENT_EVERY = 400;
inline float EVENT_LENGTH = 150;
inline float EVENT_WARNING = 60;
inline float SWARM_DRONES = 0.35f;      // added to DRONE_CHANCE in a swarm

inline float eventStart(int index)
{
    return index == 0 ? EVENT_FIRST : BOSS_DISTANCE + BOSS_ARENA + 100 + (index - 1) * EVENT_EVERY;
}

// The event whose stretch holds x, -1 for none
inline int eventIndex(float x)
{
    if (x >= EVENT_FIRST && x < EVENT_FIRST + EVENT_LENGTH)
        return 0;
    float after = BOSS_DISTANCE + BOSS_ARENA + 100;
    if (x < after)
        return -1;
    int index = 1 + static_cast<int>((x - after) / EVENT_EVERY);
    return x < eventStart(index) + EVENT_LENGTH ? index : -1;
}

// MSG_COLLISION type: an event is coming, x its kind, y its index
constexpr char EVENT_AHEAD = 'E';

#endif
