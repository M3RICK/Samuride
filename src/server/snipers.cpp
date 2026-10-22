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
    if (leader_x < SNIPER_FIRST_DISTANCE || players.empty() || (timer -= dt) > 0)
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

std::vector<uint8_t> SniperSquad::serialize() const
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(snipers.size())};

    for (const Sniper &sniper : snipers) {
        data.push_back(static_cast<uint8_t>(sniper.phase()));
        Protocol::pushU16(data, std::max(0.0f, sniper.y) * POSITION_SCALE);
    }
    return data;
}
