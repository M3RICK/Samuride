// CYBERPSYCHO: attacks, ultimates, projectiles and drones. What each runner's weapon and ultimate does: arsenal.cpp.

#include "server.hpp"
#include "../common/debug.hpp"
#include <algorithm>

// The attack key, or Berserk attacking on its own: what happens is up to the runner's weapon (arsenal.cpp)
void Server::handleAttack(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end() || !it->second.canAttack())
        return;
    if (float cooldown = it->second.weapon().attack(*this, it->second); cooldown > 0)
        it->second.startCooldown(cooldown);
}

// Only with a full gauge, out of the Attack Titan's forms; an ultimate still going may use the key itself
// (Mikasa throws a spear)
void Server::handleUltimate(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end())
        return;

    Player &player = it->second;
    if (player.ultimate().press(*this, player))
        return;
    if (!player.ultimateReady() || player.getForm() != Form::Normal)
        return;
    player.spendUltimate();

    float start = player.getX() + 1;
    float line = player.getY() + Player::HEIGHT / 2;
    std::vector<uint8_t> payload = {static_cast<uint8_t>(player.getPlayerNumber()),
                                    static_cast<uint8_t>(ultimateOf(player.getCharacter()))};
    Protocol::pushU32(payload, start * POSITION_SCALE);
    Protocol::pushU16(payload, line * POSITION_SCALE);
    broadcastToAllClients(Protocol::createPacket(MSG_ULTIMATE, payload));
    player.ultimate().fire(*this, player, start, line);
}

// What the ultimates do on their own every tick: the sweeps go on, the nukes land
void Server::updateUltimates()
{
    for (auto &[client_fd, player] : players)
        player.ultimate().act(*this, player);
}

void Server::announceShot(const Player &player, Weapon weapon)
{
    std::vector<uint8_t> shot = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(weapon)};

    broadcastToAllClients(Protocol::createPacket(MSG_SHOT, shot));
}

// Fired from the hip, flying ahead of the shooter
void Server::fireProjectile(const Player &player, Weapon weapon)
{
    WeaponStats stats = statsOf(weapon);
    float x = player.getX() + 0.9f;

    announceShot(player, weapon);
    projectiles.push_back({player.getClientFd(), x, player.getY() + 1.0f, run_speed + stats.speed, x + PROJECTILE_RANGE, stats.radius,
                           weapon});
}

int Server::strike(const Player &player, float reach)
{
    int kills = 0;

    int top = static_cast<int>(player.getY()) - 1;
    int bottom = static_cast<int>(player.getY() + Player::HEIGHT);
    int front = static_cast<int>(player.getX() + 0.5f);
    int end = static_cast<int>(player.getX() + 0.5f + reach);

    for (int x = front; x <= end; x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(player.getClientFd(), x, y);
                kills++;
            }
        }
    }
    return kills;
}

void Server::updateProjectiles()
{
    for (Projectile &shot : projectiles) {
        float from_x = shot.x;

        shot.x += shot.speed * TICK_DT;
        if (shot.weapon == Weapon::Spear) {
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
float Server::spearHit(const Projectile &shot, float from_x)
{
    for (int x = static_cast<int>(from_x); x <= static_cast<int>(shot.x); x++) {
        for (int y = std::max(0, static_cast<int>(shot.y - shot.radius)); y <= static_cast<int>(shot.y + shot.radius); y++) {
            if (isLiveDrone(x, y))
                return x + 0.5f;
        }
        if (titanInPath(x, x, shot.y))
            return x + 0.5f;
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

// Every column from `from` to `to` burns, rows top to bottom (floor to ceiling by default): drones go
// down, zappers touching those rows go out for the rest of the run, and titans between `titans_from`
// and `to` die if the rows reach them. The owner gets the bounties.
void Server::scorch(int owner_fd, float from, float to, float titans_from, int top, int bottom)
{
    int height = game_map.getHeight();
    auto owner = players.find(owner_fd);

    bottom = std::min(bottom, height - 1);
    for (int x = std::max(0, static_cast<int>(from)); x <= static_cast<int>(to); x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(owner_fd, x, y);
            } else if (game_map.getTile(x, y) == ZAPPER_TILE && !hacked_zappers.count({x, y})) {
                for (std::pair<int, int> node : zapperNodes(game_map, x, y))
                    hacked_zappers[node] = 1e9f;
            }
        }
    }
    std::erase_if(titans, [&](const Titan &titan) {
        bool reached = bottom + 1 > height - titanStats(titan.type).height;
        if (titan.x > to || titan.x < titans_from || !reached || owner == players.end())
            return false;
        owner->second.addScore(TITAN_BOUNTY);
        notifyCollision(TITAN_KILL, owner->second, titan.x, height - 1);
        return true;
    });
}

void Server::switchOffZapper(int x, int y, float seconds)
{
    float &left = hacked_zappers[{x, y}];
    left = std::max(left, seconds);
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
