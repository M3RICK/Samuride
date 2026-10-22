// The Arasaka snipers (common/snipers.hpp has their numbers): each one aims at a player on a timer,
// tracks their height, freezes, then fires along the whole row. Streamed with MSG_SNIPERS.

#ifndef SERVER_SNIPERS_HPP
    #define SERVER_SNIPERS_HPP

#include <cstdint>
#include <map>
#include <random>
#include <vector>
#include "../common/snipers.hpp"

class Player;

class SniperSquad {
public:
    void reset();
    // Aimed at a random player, at their middle
    void spawn(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players);
    void update(float dt, const std::map<int, Player> &players);
    // The beam hits everyone in its row, the whole time it lasts
    bool hit(const Player &player) const;

    // [count]([phase][y in 1/100 tile, 2 bytes])...
    std::vector<uint8_t> serialize() const;

private:
    struct Sniper {
        int target_fd;
        float y;        // its line, in tiles
        float time;     // seconds since it started aiming

        SniperPhase phase() const;
    };

    std::vector<Sniper> snipers;
    float timer = 0;    // seconds before the next one aims
};

#endif
