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

//=============================================================================
// Ultimates
//=============================================================================

// Only with a full gauge, out of the Attack Titan's forms
void Server::handleUltimate(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end())
        return;

    Player &player = it->second;
    Ultimate ultimate = ultimateOf(player.getCharacter());

    if (ultimate == Ultimate::None || !player.ultimateReady() || player.getForm() != Form::Normal)
        return;
    player.spendUltimate();

    float start = player.getX() + 1;
    std::vector<uint8_t> payload = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(ultimate)};
    Protocol::pushU32(payload, start * POSITION_SCALE);
    broadcastToAllClients(Protocol::createPacket(MSG_ULTIMATE, payload));

    if (ultimate == Ultimate::FireWave)
        fire_waves.push_back({client_fd, start, start});
    else if (ultimate == Ultimate::Nuke)
        nukes.push_back({client_fd, start + NUKE_DISTANCE, NUKE_DELAY});
}

// The fire wave sweeps on, the nukes land
void Server::updateUltimates()
{
    for (FireWave &wave : fire_waves) {
        float front = std::min(wave.start + FIRE_WAVE_RANGE, wave.front + FIRE_WAVE_SPEED * TICK_DT);
        scorch(wave.owner_fd, wave.front, front, wave.start - 1);
        wave.front = front;
    }
    std::erase_if(fire_waves, [](const FireWave &wave) { return wave.front >= wave.start + FIRE_WAVE_RANGE; });

    for (Nuke &nuke : nukes) {
        nuke.timer -= TICK_DT;
        if (nuke.timer <= 0)
            scorch(nuke.owner_fd, nuke.impact - NUKE_RADIUS, nuke.impact + NUKE_RADIUS, nuke.impact - NUKE_RADIUS);
    }
    std::erase_if(nukes, [](const Nuke &nuke) { return nuke.timer <= 0; });
}

// Every column from `from` to `to` burns from floor to ceiling: drones go down, zappers go out for the
// rest of the run, and titans between `titans_from` and `to` die. The owner gets the bounties.
void Server::scorch(int owner_fd, float from, float to, float titans_from)
{
    int height = game_map.getHeight();
    auto owner = players.find(owner_fd);

    for (int x = std::max(0, static_cast<int>(from)); x <= static_cast<int>(to); x++) {
        for (int y = 0; y < height; y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(owner_fd, x, y);
            } else if (game_map.getTile(x, y) == 'e' && !hacked_zappers.count({x, y})) {
                for (std::pair<int, int> node : zapperNodes(game_map, x, y))
                    hacked_zappers[node] = 1e9f;
            }
        }
    }
    std::erase_if(titans, [&](const Titan &titan) {
        if (titan.x > to || titan.x < titans_from || owner == players.end())
            return false;
        owner->second.addScore(TITAN_BOUNTY);
        notifyCollision(TITAN_KILL, owner->second, titan.x, height - 1);
        return true;
    });
}
