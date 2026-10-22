// Rules of the CYBERPSYCHO mode, shared by the server (who decides) and the client (who draws):
// drones to shoot down, and the runners' weapons and ultimates (runners.hpp)

#ifndef CYBERPSYCHO_HPP
    #define CYBERPSYCHO_HPP

#include <cstdint>
#include "runners.hpp"

enum class GameMode : uint8_t { Classic, Cyberpsycho };

// Eddies for a drone shot down (its tile, DRONE_TILE, is in map.hpp)
inline int DRONE_BOUNTY = 5;

#endif
