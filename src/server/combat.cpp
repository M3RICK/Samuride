// CYBERPSYCHO: attacks, projectiles and drones. The ultimates are in ultimates.cpp.

#include "server.hpp"
#include "../common/debug.hpp"
#include <algorithm>

void Server::handleAttack(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end())
        return;

    Player &player = it->second;
    Weapon weapon = player.getSpears() > 0 ? Weapon::Spear : weaponOf(player.getCharacter());
    WeaponStats stats = statsOf(weapon);

    // Lucy only hacks when there is something to hack and RAM to do it
    if (!player.canAttack() || (weapon == Weapon::Hack && !hack(client_fd, player)))
        return;
    // Pow-Pow: a burst, the rest of it fired over the next ticks (see updatePlayers); a spray in Berserk
    bool spray = weapon == Weapon::Minigun && player.isBerserk();
    player.startCooldown(spray ? MINIGUN_SPRAY_COOLDOWN : stats.cooldown);
    if (weapon == Weapon::Minigun && !spray)
        player.startBurst(MINIGUN_BURST - 1);

    if (weapon != Weapon::Blade && weapon != Weapon::Hack) {
        if (weapon == Weapon::Spear)
            player.useSpear();
        fireProjectile(client_fd, player, weapon);
        return;
    }

    std::vector<uint8_t> shot = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(weapon)};
    broadcastToAllClients(Protocol::createPacket(MSG_SHOT, shot));

    if (weapon == Weapon::Blade) {
        strike(client_fd, player, stats.reach);
        slashTitans(client_fd, player, stats.reach);
        return;
    }
}

// Fired from the hip, flying ahead of the shooter
void Server::fireProjectile(int client_fd, const Player &player, Weapon weapon)
{
    WeaponStats stats = statsOf(weapon);
    float x = player.getX() + 0.9f;
    std::vector<uint8_t> shot = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(weapon)};

    broadcastToAllClients(Protocol::createPacket(MSG_SHOT, shot));
    projectiles.push_back({client_fd, x, player.getY() + 1.0f, run_speed + stats.speed, x + PROJECTILE_RANGE, stats.radius,
                           weapon == Weapon::Spear});
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

// A drone in her way goes down, a zapper's beams go off
bool Server::hack(int client_fd, Player &player)
{
    auto [x, y] = hackTarget(game_map, player.getX(), player.getY(), Player::HEIGHT, [this](int x, int y) {
        return destroyed_drones.count({x, y}) || hacked_zappers.count({x, y});
    });

    if (player.getRam() == 0 || x < 0)
        return false;

    player.spendRam();
    if (isLiveDrone(x, y)) {
        destroyDrone(client_fd, x, y);
        return true;
    }
    for (std::pair<int, int> node : zapperNodes(game_map, x, y))
        hacked_zappers[node] = HACK_TIME;
    notifyCollision(ZAPPER_HACK, player, x, y);
    return true;
}

void Server::updateProjectiles()
{
    for (Projectile &shot : projectiles) {
        float from_x = shot.x;

        shot.x += shot.speed * TICK_DT;
        if (shot.spear) {
            float hit = spearHit(shot, from_x);
            if (hit >= 0 || shot.x >= shot.max_x) {
                spearBlast(shot.owner_fd, hit >= 0 ? hit : shot.max_x);
                shot.x = shot.max_x;
            }
        } else if (projectileHit(shot, from_x)) {
            shot.x = shot.max_x;
        }
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

// Where a spear sticks this tick: the first live drone near its line, or a titan tall enough
// to stand in its way. -1 when it flies on.
float Server::spearHit(const Projectile &shot, float from_x) const
{
    float floor = game_map.getHeight();

    for (int x = static_cast<int>(from_x); x <= static_cast<int>(shot.x); x++) {
        for (int y = std::max(0, static_cast<int>(shot.y - shot.radius)); y <= static_cast<int>(shot.y + shot.radius); y++) {
            if (isLiveDrone(x, y))
                return x + 0.5f;
        }
        for (const Titan &titan : titans) {
            TitanStats stats = titanStats(titan.type);
            if (x + 1 > titan.x && x < titan.x + stats.width && shot.y > floor - stats.height)
                return x + 0.5f;
        }
    }
    return -1;
}

// A lightning column from floor to ceiling: the drones, zappers and titans in it go down
void Server::spearBlast(int owner_fd, float x)
{
    auto owner = players.find(owner_fd);

    if (owner != players.end())
        notifyCollision(SPEAR_BLAST, owner->second, x, 0);
    scorch(owner_fd, x - SPEAR_BLAST_RADIUS, x + SPEAR_BLAST_RADIUS, x - SPEAR_BLAST_RADIUS - 2);
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
