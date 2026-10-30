// The titans on the server (see titans.hpp)

#include "titans.hpp"
#include "player.hpp"
#include "../common/protocol.hpp"
#include <algorithm>

void Horde::reset()
{
    titans.clear();
    // The first one comes soon after TITAN_FIRST_DISTANCE
    timer = 2;
}

// ponytail: a titan can pass under a zapper that closes the gap above it; add a check
// against the map ahead if such deaths feel unfair
void Horde::spawn(float leader_x, float dt, std::mt19937 &rng, const std::function<bool(const Titan &)> &fits)
{
    if (leader_x < TITAN_FIRST_DISTANCE || (timer -= dt) > 0)
        return;

    std::uniform_real_distribution<float> interval(TITAN_MIN_INTERVAL, std::max(TITAN_MIN_INTERVAL, TITAN_MAX_INTERVAL));
    std::uniform_int_distribution<int> type(0, TITAN_TYPES - 1);
    Titan titan{static_cast<TitanType>(type(rng)), leader_x + TITAN_SPAWN_AHEAD};

    if (!fits(titan))
        return;
    timer = interval(rng);
    titans.push_back(titan);
}

void Horde::update(float rear_x, float dt)
{
    for (Titan &titan : titans)
        titan.x -= titanStats(titan.type).speed * dt;
    std::erase_if(titans, [rear_x](const Titan &titan) { return titan.x < rear_x - 10; });
}

bool Horde::hit(const Player &player, float floor) const
{
    return std::ranges::any_of(titans, [&](const Titan &titan) {
        TitanStats stats = titanStats(titan.type);
        bool overlap_x = player.getX() + 0.8f > titan.x && player.getX() + 0.2f < titan.x + stats.width;
        bool overlap_y = player.getY() + Player::HEIGHT > floor - stats.height + 0.3f;
        return overlap_x && overlap_y;
    });
}

bool Horde::inPath(float from_x, float to_x, float y, float floor) const
{
    return std::ranges::any_of(titans, [&](const Titan &titan) {
        TitanStats stats = titanStats(titan.type);
        return to_x + 1 > titan.x && from_x < titan.x + stats.width && y > floor - stats.height;
    });
}

std::vector<TitanState> Horde::states() const
{
    std::vector<TitanState> states;

    for (const Titan &titan : titans)
        states.push_back({titan.type, titan.x});
    return states;
}
