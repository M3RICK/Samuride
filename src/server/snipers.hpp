// The Arasaka snipers (common/snipers.hpp has their numbers): each one aims at a player on a timer,
// tracks their height, freezes, then fires a bullet along the row. Streamed with MSG_SNIPERS.

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
    // dt 0 while the world stands still: the sights stop following, a bullet hangs in the air
    void update(float dt, float leader_x, const std::map<int, Player> &players);
    // The bullet hits whoever it reaches in its row
    bool hit(const Player &player) const;

    // What the clients see of them, for MSG_SNIPERS
    std::vector<SniperState> states() const;

private:
    struct Sniper {
        int target_fd;
        float x;        // where its bullet left from, in tiles
        float y;        // its line, in tiles
        float time;     // seconds since it started aiming
        float aim;      // seconds its sight holds still before the shot (longer on Cold Blood)

        SniperPhase phase() const;
        // Its bullet's nose
        float bulletX() const { return x - (time - SNIPER_TRACK_TIME - aim) * SNIPER_BULLET_SPEED; }
    };

    std::vector<Sniper> snipers;
    float timer = 0;    // seconds before the next one aims
};

#endif
