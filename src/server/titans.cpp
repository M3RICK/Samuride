/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include <algorithm>

// Titans: spawned on a timer once the run is long enough, moved every tick,
// streamed to clients with MSG_TITANS

void Server::resetTitans()
{
    titans.clear();
    // The first one comes soon after TITAN_FIRST_DISTANCE
    titan_timer = 2;
}

// ponytail: a titan can pass under a zapper that closes the gap above it; add a check
// against the map ahead if such deaths feel unfair
void Server::spawnTitans(float leader_x)
{
    if (leader_x < TITAN_FIRST_DISTANCE)
        return;

    titan_timer -= TICK_DT;
    if (titan_timer > 0)
        return;

    std::uniform_real_distribution<float> interval(TITAN_MIN_INTERVAL, TITAN_MAX_INTERVAL);
    std::uniform_int_distribution<int> type(0, TITAN_TYPES - 1);

    titan_timer = interval(rng);
    titans.push_back({static_cast<TitanType>(type(rng)), leader_x + TITAN_SPAWN_AHEAD});
}

void Server::updateTitans(float rear_x)
{
    for (Titan &titan : titans)
        titan.x -= titanStats(titan.type).speed * TICK_DT;

    // Gone once behind everyone
    std::erase_if(titans, [rear_x](const Titan &titan) { return titan.x < rear_x - 10; });
}

// The player's body against the titan's: both stand on, or fly above, the same floor
bool Server::hitByTitan(const Player &player) const
{
    float floor = game_map.getHeight();

    for (const Titan &titan : titans) {
        TitanStats stats = titanStats(titan.type);
        bool overlap_x = player.getX() + 0.8f > titan.x && player.getX() + 0.2f < titan.x + stats.width;
        bool overlap_y = player.getY() + Player::HEIGHT > floor - stats.height + 0.3f;

        if (overlap_x && overlap_y)
            return true;
    }
    return false;
}

// CYBERPSYCHO: a strike within reach takes the titan down
void Server::slashTitans(int client_fd, const Player &player, float reach)
{
    auto owner = players.find(client_fd);

    std::erase_if(titans, [&](const Titan &titan) {
        bool in_reach = titan.x > player.getX() - 0.5f && titan.x < player.getX() + 0.5f + reach;
        if (!in_reach || owner == players.end())
            return false;
        owner->second.addScore(TITAN_BOUNTY);
        notifyCollision(TITAN_KILL, owner->second, titan.x, game_map.getHeight() - 1);
        return true;
    });
}

// [count]([type][x in 1/100 tile, 4 bytes])...
void Server::sendTitans()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(titans.size())};

    for (const Titan &titan : titans) {
        data.push_back(static_cast<uint8_t>(titan.type));
        Protocol::pushU32(data, std::max(0.0f, titan.x) * POSITION_SCALE);
    }
    broadcastToAllClients(Protocol::createPacket(MSG_TITANS, data));
}
