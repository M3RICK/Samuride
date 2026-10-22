// Arasaka snipers: aimed on a timer at one player, ticked down, streamed to clients with MSG_SNIPERS

#include "server.hpp"
#include <algorithm>

void Server::resetSnipers()
{
    snipers.clear();
    sniper_timer = 2;
}

// Aimed at a random player, at their middle
void Server::spawnSnipers(float leader_x)
{
    if (leader_x < SNIPER_FIRST_DISTANCE || players.empty())
        return;

    sniper_timer -= TICK_DT;
    if (sniper_timer > 0)
        return;

    std::uniform_real_distribution<float> interval(SNIPER_MIN_INTERVAL, SNIPER_MAX_INTERVAL);
    auto target = std::next(players.begin(), std::uniform_int_distribution<size_t>(0, players.size() - 1)(rng));

    sniper_timer = interval(rng);
    snipers.push_back({target->first, target->second.getY() + Player::HEIGHT / 2, 0});
}

void Server::updateSnipers()
{
    for (Sniper &sniper : snipers) {
        sniper.time += TICK_DT;

        auto target = players.find(sniper.target_fd);
        if (sniper.phase() == SniperPhase::Tracking && target != players.end())
            sniper.y = target->second.getY() + Player::HEIGHT / 2;
    }

    std::erase_if(snipers, [](const Sniper &sniper) {
        return sniper.time > SNIPER_TRACK_TIME + SNIPER_AIM_TIME + SNIPER_FIRE_TIME;
    });
}

// The beam hits everyone in its row, the whole time it lasts: a shield takes it, then its grace does
bool Server::hitBySniper(const Player &player) const
{
    return std::ranges::any_of(snipers, [&](const Sniper &sniper) {
        return sniper.phase() == SniperPhase::Firing && sniperHits(sniper.y, player.getY());
    });
}

// [count]([phase][y in 1/100 tile, 2 bytes])...
void Server::sendSnipers()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(snipers.size())};

    for (const Sniper &sniper : snipers) {
        data.push_back(static_cast<uint8_t>(sniper.phase()));
        Protocol::pushU16(data, std::max(0.0f, sniper.y) * POSITION_SCALE);
    }
    broadcastToAllClients(Protocol::createPacket(MSG_SNIPERS, data));
}
