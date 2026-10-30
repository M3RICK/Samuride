// CYBERPSYCHO: attacks, ultimates, projectiles and drones. What each runner's weapon and ultimate does: arsenal.cpp.

#include "server.hpp"
#include "../common/debug.hpp"
#include <algorithm>

// The attack key, or Berserk attacking on its own: what happens is up to the runner's weapon (arsenal.cpp).
// Not in the Attack Titan's forms: the titan, or the mount, only tramples
void Server::handleAttack(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end() || !it->second.canAttack() ||
        it->second.getForm() != Form::Normal)
        return;
    if (float cooldown = it->second.weapon().attack(*this, it->second); cooldown > 0)
        it->second.startCooldown(cooldown);
}

void Server::releaseAttack(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end() ||
        it->second.getForm() != Form::Normal)
        return;
    if (float cooldown = it->second.weapon().release(*this, it->second); cooldown > 0)
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
    UltimateEvent fired{player.getPlayerNumber(), ultimateOf(player.getCharacter()), start, line};
    broadcastToAllClients(Protocol::createPacket(MSG_ULTIMATE, Message::encode(fired)));
    player.ultimate().fire(*this, player, start, line);
    if (!player.ultimate().travels())
        hitBoss(player, BOSS_ULTIMATE_DAMAGE);
}

// What the ultimates do on their own every tick: the sweeps go on, the nukes land
void Server::updateUltimates()
{
    for (auto &[client_fd, player] : players)
        player.ultimate().act(*this, player);
}

void Server::announceShot(const Player &player, Weapon weapon)
{
    ShotEvent shot{player.getPlayerNumber(), weapon};

    broadcastToAllClients(Protocol::createPacket(MSG_SHOT, Message::encode(shot)));
}

// Fired from the hip, flying ahead of the shooter
void Server::fireProjectile(const Player &player, Weapon weapon)
{
    announceShot(player, weapon);
    fireProjectile(player, weapon, player.getY());
}

void Server::fireProjectile(const Player &player, Weapon weapon, float y)
{
    WeaponStats stats = statsOf(weapon);
    float x = player.getX() + 0.9f;

    projectiles.push_back({player.getClientFd(), x, y + 1.0f, run_speed + stats.speed, x + PROJECTILE_RANGE, stats.radius, weapon});
}

// The live drones in a strike's box: from the player's front to `reach` ahead, over a body whose top is at y
static std::vector<std::pair<int, int>> dronesInReach(const Server &server, const Player &player, float reach, float y)
{
    std::vector<std::pair<int, int>> drones;
    int top = static_cast<int>(y) - 1;
    int bottom = static_cast<int>(y + Player::HEIGHT);
    int front = static_cast<int>(player.getX() + 0.5f);
    int end = static_cast<int>(player.getX() + 0.5f + reach);

    for (int x = front; x <= end; x++)
        for (int y = std::max(0, top); y <= bottom; y++)
            if (server.isLiveDrone(x, y))
                drones.emplace_back(x, y);
    return drones;
}

static auto titanReached(const Player &player, float reach)
{
    return [&player, reach](const Horde::Titan &titan) {
        return titan.x > player.getX() - 0.5f && titan.x < player.getX() + 0.5f + reach;
    };
}

int Server::strike(const Player &player, float reach, float y)
{
    std::vector<std::pair<int, int>> drones = dronesInReach(*this, player, reach, y);

    for (auto [x, y] : drones)
        destroyDrone(player.getClientFd(), x, y);
    return static_cast<int>(drones.size());
}

// His body starts half a tile before his x
static bool bossReached(const Boss &boss, const Player &player, float reach)
{
    return boss.fighting() && boss.x() - 0.5f < player.getX() + 0.5f + reach && boss.x() + 2.5f > player.getX();
}

bool Server::inReach(const Player &player, float reach, float y) const
{
    return !dronesInReach(*this, player, reach, y).empty() || titans.any(titanReached(player, reach)) ||
           (mode == GameMode::Cyberpsycho && bossReached(boss, player, reach));
}

bool Server::slashBoss(Player &player, float reach)
{
    return bossReached(boss, player, reach) && hitBoss(player, BOSS_SLASH_DAMAGE);
}

void Server::updateProjectiles()
{
    for (Projectile &shot : projectiles) {
        float from_x = shot.x;

        shot.x += shot.speed * TICK_DT;
        auto owner = players.find(shot.owner_fd);
        if (boss.fighting() && shot.x >= boss.x() && owner != players.end()) {
            bool spear = shot.weapon == Weapon::Spear;
            if (spear)
                spearBlast(shot.owner_fd, boss.x());
            hitBoss(owner->second, spear ? BOSS_SPEAR_DAMAGE : BOSS_SHOT_DAMAGE);
            shot.x = shot.max_x;
        } else if (shot.weapon == Weapon::Spear) {
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
        if (titans.inPath(x, x, shot.y, game_map.getHeight()))
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
    if (owner == players.end())
        return;
    auto reached = [&](const Horde::Titan &titan) {
        return titan.x <= to && titan.x >= titans_from && bottom + 1 > height - titanStats(titan.type).height;
    };
    for (float x : titans.takeDown(reached))
        titanDown(owner->second, x);
}

void Server::slashTitans(Player &player, float reach)
{
    for (float x : titans.takeDown(titanReached(player, reach)))
        titanDown(player, x);
}

bool Server::hitBoss(Player &player, int damage)
{
    if (mode != GameMode::Cyberpsycho || !boss.fighting())
        return false;
    if (boss.damage(damage)) {
        player.addScore(BOSS_BOUNTY);
        notifyCollision(BOSS_KILL, player, static_cast<int>(boss.x()), static_cast<int>(game_map.getHeight() / 2));
    }
    return true;
}

void Server::titanDown(Player &player, float x)
{
    player.addScore(TITAN_BOUNTY);
    notifyCollision(TITAN_KILL, player, x, game_map.getHeight() - 1);
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

    owner->second.addScore(wantedBounty(owner->second.weapon().droneDown(), owner->second.wantedStars()));
    owner->second.raiseWanted();
    notifyCollision(DRONE_TILE, owner->second, x, y);

    float pinata = pinataChance(owner->second.getUpgrades());
    if (droneDrop(x, y) == EMPTY_TILE && pinata > 0 && std::uniform_real_distribution<float>(0, 1)(rng) < pinata) {
        pinata_drops.insert({x, y});
        notifyCollision(PINATA_DROP, owner->second, x, y);
    }
    DEBUG_LOG("Player " + std::to_string(owner->second.getPlayerNumber()) + " destroyed a drone");
}
