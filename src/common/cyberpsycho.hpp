// Rules of the CYBERPSYCHO mode, shared by the server (who decides) and the client (who draws):
// drones to shoot down, and the runners' weapons and ultimates (runners.hpp)

#ifndef CYBERPSYCHO_HPP
    #define CYBERPSYCHO_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "runners.hpp"

enum class GameMode : uint8_t { Classic, Cyberpsycho };

// Eddies for a drone shot down (its tile, DRONE_TILE, is in map.hpp)
inline int DRONE_BOUNTY = 5;

// NCPD wanted level, GTA style: each drone down heats the NCPD up, WANTED_KILLS_PER_STAR kills a star,
// and after WANTED_CALM_TIME seconds without one the heat cools by WANTED_COOLING kills' worth a second. Each star adds drones to the map ahead,
// sends the snipers more often and makes every drone pay more: you pick your own difficulty.
constexpr int WANTED_MAX_STARS = 5;         // sent in 3 bits
inline int WANTED_KILLS_PER_STAR = 5;
inline float WANTED_CALM_TIME = 5;
inline float WANTED_COOLING = 0.4f;
inline float WANTED_BOUNTY_PER_STAR = 0.2f;   // drone bounty x1.2 a star
inline float WANTED_DRONES_PER_STAR = 0.03f;  // added to DRONE_CHANCE
inline float WANTED_SNIPERS_PER_STAR = 0.1f;  // snipers that much more often a star (x1.5 at 5)

inline int wantedStars(float heat)
{
    return std::clamp(static_cast<int>(heat / WANTED_KILLS_PER_STAR), 0, WANTED_MAX_STARS);
}
// At 5 stars the heat goes one star higher, so the top one doesn't drop at the first breath
inline float wantedHeatCap()
{
    return (WANTED_MAX_STARS + 1) * WANTED_KILLS_PER_STAR - 0.01f;
}
inline int wantedBounty(int bounty, int stars)
{
    return static_cast<int>(std::lround(bounty * (1 + WANTED_BOUNTY_PER_STAR * stars)));
}

#endif
