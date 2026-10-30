// The horde of titans charging along the floor (common/titans.hpp has their kinds and numbers): spawned on a
// timer once the run is long enough, moved every tick, streamed to the clients with MSG_TITANS.
// Whoever takes one down gets its bounty from the server.

#ifndef SERVER_TITANS_HPP
    #define SERVER_TITANS_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>
#include "../common/messages.hpp"

class Player;

class Horde {
public:
    struct Titan {
        TitanType type;
        float x;        // left edge, in tiles
    };

    void reset();
    // Due, it waits for `fits` to leave the runners a way past it
    void spawn(float leader_x, float dt, std::mt19937 &rng, const std::function<bool(const Titan &)> &fits);
    // Gone once far behind everyone
    void update(float rear_x, float dt);

    // The player's body against a titan's: both stand on, or fly above, the same floor
    bool hit(const Player &player, float floor) const;
    // A shot flying along line y between the columns from_x and to_x crossed a titan tall enough
    bool inPath(float from_x, float to_x, float y, float floor) const;
    template <typename Reached>
    bool any(Reached reached) const { return std::ranges::any_of(titans, reached); }
    // Takes down every titan `reached` says yes to; returns where they stood
    template <typename Reached>
    std::vector<float> takeDown(Reached reached)
    {
        std::vector<float> down;
        std::erase_if(titans, [&](const Titan &titan) {
            if (!reached(titan))
                return false;
            down.push_back(titan.x);
            return true;
        });
        return down;
    }

    // What the clients see of them, for MSG_TITANS
    std::vector<TitanState> states() const;

private:
    std::vector<Titan> titans;
    float timer = 0;    // seconds before the next one
};

#endif
