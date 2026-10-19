#include "server.hpp"
#include "../common/debug.hpp"
#include <algorithm>

// CYBERPSYCHO: attacks, projectiles and drones

void Server::handleAttack(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end())
        return;

    Player &player = it->second;
    Weapon weapon = weaponOf(player.getCharacter());
    WeaponStats stats = statsOf(weapon);

    // Lucy only hacks when there is something to hack and RAM to do it
    if (!player.canAttack() || (weapon == Weapon::Hack && !hack(client_fd, player)))
        return;
    player.startCooldown(stats.cooldown);

    std::vector<uint8_t> shot = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(weapon)};
    broadcastToAllClients(Protocol::createPacket(MSG_SHOT, shot));

    if (weapon == Weapon::Blade) {
        strike(client_fd, player, stats.reach);
        slashTitans(client_fd, player, stats.reach);
        return;
    }
    if (weapon == Weapon::Hack)
        return;

    // Fired from the hip, flying ahead of the shooter
    float x = player.getX() + 0.9f;
    projectiles.push_back({client_fd, x, player.getY() + 1.0f, run_speed + stats.speed, x + PROJECTILE_RANGE, stats.radius});
}

// Every live drone within reach in front of the player goes down
void Server::strike(int client_fd, const Player &player, float reach)
{
    int top = static_cast<int>(player.getY()) - 1;
    int bottom = static_cast<int>(player.getY() + Player::HEIGHT);
    int front = static_cast<int>(player.getX() + 0.5f);
    int end = static_cast<int>(player.getX() + 0.5f + reach);

    for (int x = front; x <= end; x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (isLiveDrone(x, y))
                destroyDrone(client_fd, x, y);
        }
    }
}

// The first live drone or zapper ahead, column by column: a drone goes down, a zapper's beams go off
bool Server::hack(int client_fd, Player &player)
{
    int front = static_cast<int>(player.getX() + 0.5f);

    if (player.getRam() == 0)
        return false;

    for (int x = front; x <= front + HACK_RANGE; x++) {
        for (int y = 0; y < static_cast<int>(game_map.getHeight()); y++) {
            if (isLiveDrone(x, y)) {
                player.spendRam();
                destroyDrone(client_fd, x, y);
                return true;
            }
            if (game_map.getTile(x, y) == 'e' && !hacked_zappers.count({x, y})) {
                player.spendRam();
                for (std::pair<int, int> node : zapperNodes(game_map, x, y))
                    hacked_zappers[node] = HACK_TIME;
                notifyCollision(ZAPPER_HACK, player, x, y);
                return true;
            }
        }
    }
    return false;
}

void Server::updateProjectiles()
{
    for (Projectile &shot : projectiles) {
        float from_x = shot.x;

        shot.x += shot.speed * TICK_DT;
        if (projectileHit(shot, from_x))
            shot.x = shot.max_x;
    }
    std::erase_if(projectiles, [](const Projectile &shot) { return shot.x >= shot.max_x; });
}

// The first live drone the shot went through this tick, within its radius of its line.
// Every column crossed is checked: a fast shot can't skip over a drone.
bool Server::projectileHit(const Projectile &shot, float from_x)
{
    int top = static_cast<int>(shot.y - shot.radius);
    int bottom = static_cast<int>(shot.y + shot.radius);

    for (int x = static_cast<int>(from_x); x <= static_cast<int>(shot.x); x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(shot.owner_fd, x, y);
                return true;
            }
        }
    }
    return false;
}

bool Server::isLiveDrone(int x, int y) const
{
    return x >= 0 && y >= 0 && game_map.getTile(x, y) == DRONE_TILE && !destroyed_drones.count({x, y});
}

// The owner may have left: the drone still goes down, nobody gets the bounty
void Server::destroyDrone(int owner_fd, int x, int y)
{
    auto owner = players.find(owner_fd);

    destroyed_drones.insert({x, y});
    if (owner == players.end())
        return;

    owner->second.addScore(DRONE_BOUNTY);
    notifyCollision(DRONE_TILE, owner->second, x, y);
    DEBUG_LOG("Player " + std::to_string(owner->second.getPlayerNumber()) + " destroyed a drone");
}
