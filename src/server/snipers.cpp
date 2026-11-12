// The Arasaka snipers on the server (see snipers.hpp)

#include "snipers.hpp"
#include "player.hpp"
#include "../common/protocol.hpp"
#include <algorithm>

SniperPhase SniperSquad::Sniper::phase() const
{
    if (time < SNIPER_TRACK_TIME)
        return SniperPhase::Tracking;
    return time < SNIPER_TRACK_TIME + aim ? SniperPhase::Aiming : SniperPhase::Firing;
}

void SniperSquad::reset()
{
    snipers.clear();
    timer = 2;
}

void SniperSquad::spawn(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players,
                        const std::function<bool(const Player &)> &fits)
{
    // Never two sights at once, however short the interval: one waits for the other's shot to be over
    if (leader_x < SNIPER_FIRST_DISTANCE || players.empty() || (timer -= dt) > 0 || !snipers.empty())
        return;

    std::uniform_real_distribution<float> interval(SNIPER_MIN_INTERVAL, std::max(SNIPER_MIN_INTERVAL, SNIPER_MAX_INTERVAL));
    auto target = std::next(players.begin(), std::uniform_int_distribution<size_t>(0, players.size() - 1)(rng));

    if (!fits(target->second))
        return;
    timer = interval(rng);
    float aim = SNIPER_AIM_TIME + (target->second.hasBoon(Boon::ColdBlood) ? COLD_BLOOD_AIM : 0);
    snipers.push_back({target->first, 0, target->second.getY() + Player::HEIGHT / 2, 0, aim});
}

void SniperSquad::update(float dt, float leader_x, const std::map<int, Player> &players)
{
    if (dt <= 0)
        return;
    for (Sniper &sniper : snipers) {
        bool fired = sniper.phase() == SniperPhase::Firing;
        sniper.time += dt;

        auto target = players.find(sniper.target_fd);
        if (sniper.phase() == SniperPhase::Tracking && target != players.end())
            sniper.y = target->second.getY() + Player::HEIGHT / 2;
        if (!fired && sniper.phase() == SniperPhase::Firing)
            sniper.x = leader_x + SNIPER_BULLET_AHEAD;
    }

    std::erase_if(snipers, [](const Sniper &sniper) {
        return sniper.time > SNIPER_TRACK_TIME + sniper.aim + SNIPER_FIRE_TIME;
    });
}

bool SniperSquad::hit(const Player &player) const
{
    return std::ranges::any_of(snipers, [&](const Sniper &sniper) {
        return sniper.phase() == SniperPhase::Firing && sniperHits(sniper.bulletX(), sniper.y, player.getX(), player.getY());
    });
}

std::vector<SniperState> SniperSquad::states() const
{
    std::vector<SniperState> states;

    for (const Sniper &sniper : snipers)
        states.push_back({sniper.phase(), sniper.bulletX(), sniper.y});
    return states;
}
