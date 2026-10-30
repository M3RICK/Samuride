// The Arasaka snipers (common/snipers.hpp has their numbers): each one aims at a player on a timer,
// tracks their height, freezes, then fires along the whole row. Streamed with MSG_SNIPERS.

#ifndef SERVER_SNIPERS_HPP
    #define SERVER_SNIPERS_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <random>
#include <vector>
#include "../common/messages.hpp"

class Player;

class SniperSquad {
public:
    void reset();
    // Aimed at a random player, at their middle; due, it waits for `fits` to leave them a way to sidestep
    void spawn(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players,
               const std::function<bool(const Player &)> &fits);
    bool active() const { return !snipers.empty(); }
    void update(float dt, const std::map<int, Player> &players);
    // The beam hits everyone in its row, the whole time it lasts
    bool hit(const Player &player) const;

    // What the clients see of them, for MSG_SNIPERS
    std::vector<SniperState> states() const;

private:
    struct Sniper {
        int target_fd;
        float y;        // its line, in tiles
        float time;     // seconds since it started aiming
        float aim;      // seconds its sight holds still before the shot (longer on Cold Blood)

        SniperPhase phase() const;
    };

    std::vector<Sniper> snipers;
    float timer = 0;    // seconds before the next one aims
};

#endif
