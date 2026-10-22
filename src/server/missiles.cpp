// Missiles: spawned on a timer at one player's height, moved every tick, streamed to clients with
// MSG_MISSILES

#include "server.hpp"
#include <algorithm>

void Server::resetMissiles()
{
    missiles.clear();
    missile_timer = 2;
}

// Aimed at a random player, at their middle
void Server::spawnMissiles(float leader_x)
{
    if (leader_x < MISSILE_FIRST_DISTANCE || players.empty())
        return;

    missile_timer -= TICK_DT;
    if (missile_timer > 0)
        return;

    std::uniform_real_distribution<float> interval(MISSILE_MIN_INTERVAL, MISSILE_MAX_INTERVAL);
    auto target = std::next(players.begin(), std::uniform_int_distribution<size_t>(0, players.size() - 1)(rng));
    const Player &player = target->second;

    missile_timer = interval(rng);
    missiles.push_back({target->first, player.getX() + MISSILE_SPAWN_AHEAD, player.getY() + Player::HEIGHT / 2,
                        MISSILE_TRACK_TIME});
}

void Server::updateMissiles(float rear_x)
{
    for (Missile &missile : missiles) {
        missile.x -= MISSILE_SPEED * TICK_DT;
        missile.track -= TICK_DT;

        auto target = players.find(missile.target_fd);
        if (missile.track > 0 && target != players.end())
            missile.y = target->second.getY() + Player::HEIGHT / 2;
    }

    // Gone once behind everyone
    std::erase_if(missiles, [rear_x](const Missile &missile) { return missile.x < rear_x - 10; });
}

// A missile that hits blows up, shield or not
bool Server::hitByMissile(const Player &player)
{
    auto hit = std::ranges::find_if(missiles, [&](const Missile &missile) {
        return missileHits(missile.x, missile.y, player.getX(), player.getY());
    });

    if (hit == missiles.end())
        return false;
    missiles.erase(hit);
    return true;
}

// [count]([locked][x in 1/100 tile, 4 bytes][y in 1/100 tile, 2 bytes])...
void Server::sendMissiles()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(missiles.size())};

    for (const Missile &missile : missiles) {
        data.push_back(missile.track <= 0);
        Protocol::pushU32(data, std::max(0.0f, missile.x) * POSITION_SCALE);
        Protocol::pushU16(data, std::max(0.0f, missile.y) * POSITION_SCALE);
    }
    broadcastToAllClients(Protocol::createPacket(MSG_MISSILES, data));
}
