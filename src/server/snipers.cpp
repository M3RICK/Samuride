// The Arasaka snipers on the server (see snipers.hpp)

#include "snipers.hpp"
#include "player.hpp"
#include "../common/protocol.hpp"
#include <algorithm>

SniperPhase SniperSquad::Sniper::phase() const
{
    if (time < SNIPER_TRACK_TIME)
        return SniperPhase::Tracking;
    return time < SNIPER_TRACK_TIME + SNIPER_AIM_TIME ? SniperPhase::Aiming : SniperPhase::Firing;
}

void SniperSquad::reset()
{
    snipers.clear();
    timer = 2;
}

void SniperSquad::spawn(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players)
{
    // Never two sights at once, however short the interval: one waits for the other's shot to be over
    if (leader_x < SNIPER_FIRST_DISTANCE || players.empty() || (timer -= dt) > 0 || !snipers.empty())
        return;

    std::uniform_real_distribution<float> interval(SNIPER_MIN_INTERVAL, std::max(SNIPER_MIN_INTERVAL, SNIPER_MAX_INTERVAL));
    auto target = std::next(players.begin(), std::uniform_int_distribution<size_t>(0, players.size() - 1)(rng));

    timer = interval(rng);
    snipers.push_back({target->first, target->second.getY() + Player::HEIGHT / 2, 0});
}

void SniperSquad::update(float dt, const std::map<int, Player> &players)
{
    for (Sniper &sniper : snipers) {
        sniper.time += dt;

        auto target = players.find(sniper.target_fd);
        if (sniper.phase() == SniperPhase::Tracking && target != players.end())
            sniper.y = target->second.getY() + Player::HEIGHT / 2;
    }

    std::erase_if(snipers, [](const Sniper &sniper) {
        return sniper.time > SNIPER_TRACK_TIME + SNIPER_AIM_TIME + SNIPER_FIRE_TIME;
    });
}

bool SniperSquad::hit(const Player &player) const
{
    return std::ranges::any_of(snipers, [&](const Sniper &sniper) {
        return sniper.phase() == SniperPhase::Firing && sniperHits(sniper.y, player.getY());
    });
}

std::vector<SniperState> SniperSquad::states() const
{
    std::vector<SniperState> states;

    for (const Sniper &sniper : snipers)
        states.push_back({sniper.phase(), sniper.y});
    return states;
}
