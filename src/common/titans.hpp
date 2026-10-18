/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef TITANS_HPP
    #define TITANS_HPP

#include <cstdint>

// Titans charge along the floor from the right: fly over them.
// Shared by the server (who moves them and checks hits) and the client (who draws them).

enum class TitanType : uint8_t { Female, Armored };

constexpr int TITAN_TYPES = 2;

struct TitanStats {
    float height;   // in tiles, standing on the floor
    float width;
    float speed;    // tiles per second toward the player, on top of the player's own run
};

inline TitanStats titanStats(TitanType type)
{
    if (type == TitanType::Armored)
        return {4.0f, 2.0f, 5};
    return {4.5f, 1.6f, 9};
}

// No titan before this distance, then one every TITAN_INTERVAL seconds or so
constexpr float TITAN_FIRST_DISTANCE = 150;
constexpr float TITAN_MIN_INTERVAL = 7;
constexpr float TITAN_MAX_INTERVAL = 14;

// They appear this far ahead of the leader: about a second and a half of warning
constexpr float TITAN_SPAWN_AHEAD = 40;

// CYBERPSYCHO: Mikasa's blade takes the nape, for a fat bounty
constexpr int TITAN_BOUNTY = 20;

// MSG_COLLISION types: a titan ran a player down, or a player cut a titan down
constexpr char TITAN_HIT = 't';
constexpr char TITAN_KILL = 'k';

#endif
