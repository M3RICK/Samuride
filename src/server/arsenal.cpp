// The runners' weapons and ultimates on the server (see arsenal.hpp). The clients draw them in
// render/shots.cpp and render/ultimates.cpp.

#include "arsenal.hpp"
#include "server.hpp"

std::unique_ptr<RunnerWeapon> makeWeapon(Weapon id)
{
    switch (id) {
        case Weapon::Minigun:
            return std::make_unique<Minigun>();
        case Weapon::Blade:
            return std::make_unique<Blade>();
        case Weapon::Hack:
            return std::make_unique<Hack>();
        default:
            return std::make_unique<Gun>();
    }
}

std::unique_ptr<RunnerUltimate> makeUltimate(Ultimate id)
{
    switch (id) {
        case Ultimate::Nuke:
            return std::make_unique<Nuke>();
        case Ultimate::Breach:
            return std::make_unique<Breach>();
        case Ultimate::ThunderSpears:
            return std::make_unique<ThunderSpears>();
        case Ultimate::Fishbones:
            return std::make_unique<Fishbones>();
        default:
            return std::make_unique<FireWave>();
    }
}

//=============================================================================
// Weapons
//=============================================================================

float Gun::attack(Server &server, Player &player)
{
    server.fireProjectile(player, Weapon::Gun);
    return statsOf(Weapon::Gun).cooldown;
}

// A burst, the rest of it fired over the next ticks (act); a spray in Berserk
float Minigun::attack(Server &server, Player &player)
{
    server.fireProjectile(player, Weapon::Minigun);
    if (player.isBerserk())
        return MINIGUN_SPRAY_COOLDOWN;
    startBurst();
    return statsOf(Weapon::Minigun).cooldown;
}

void Minigun::act(Server &server, Player &player)
{
    if (burstShotDue(Server::TICK_DT))
        server.fireProjectile(player, Weapon::Minigun);
}

bool Minigun::burstShotDue(float dt)
{
    if (burst_left == 0 || (burst_timer -= dt) > 0)
        return false;
    burst_left--;
    burst_timer = MINIGUN_BURST_GAP;
    return true;
}

float Blade::attack(Server &server, Player &player)
{
    WeaponStats stats = statsOf(Weapon::Blade);

    server.announceShot(player, Weapon::Blade);
    server.strike(player, stats.reach);
    server.slashTitans(player, stats.reach);
    return stats.cooldown;
}

// Only when there is something in her way and RAM to hack it: a drone goes down, a zapper's beams go off
float Hack::attack(Server &server, Player &player)
{
    auto [x, y] = hackTarget(server.map(), player.getX(), player.getY(), Player::HEIGHT,
                             [&server](int x, int y) { return server.isOff(x, y); });

    if (ram == 0 || x < 0)
        return 0;

    ram--;
    if (server.isLiveDrone(x, y)) {
        server.destroyDrone(player.getClientFd(), x, y);
        bounce(server, player, x, y);
    } else {
        for (std::pair<int, int> node : zapperNodes(server.map(), x, y))
            server.switchOffZapper(node.first, node.second, HACK_TIME);
        server.notifyCollision(ZAPPER_HACK, player, x, y);
    }
    server.announceShot(player, Weapon::Hack);
    return statsOf(Weapon::Hack).cooldown;
}

// The nearest live drone around the one just hacked goes down too, if there is one close enough
void Hack::bounce(Server &server, const Player &player, int from_x, int from_y)
{
    int reach = static_cast<int>(HACK_BOUNCE_RANGE);
    std::pair<int, int> target = {-1, -1};
    int best = reach * reach;

    for (int x = std::max(0, from_x - reach); x <= from_x + reach; x++) {
        for (int y = std::max(0, from_y - reach); y <= from_y + reach; y++) {
            int distance = (x - from_x) * (x - from_x) + (y - from_y) * (y - from_y);
            if (distance <= best && server.isLiveDrone(x, y)) {
                best = distance;
                target = {x, y};
            }
        }
    }
    if (target.first < 0)
        return;
    // Before the drone goes down: the clients draw the arc from the last drone this player took
    server.notifyCollision(HACK_BOUNCE, player, target.first, target.second);
    server.destroyDrone(player.getClientFd(), target.first, target.second);
}

// A charge comes back every RAM_RECHARGE, faster in Berserk
void Hack::tick(const Player &player, float dt)
{
    if (ram < RAM_MAX && (ram_timer -= dt * (player.isBerserk() ? BERSERK_RAM_RATE : 1)) <= 0) {
        ram++;
        ram_timer = RAM_RECHARGE;
    }
}

//=============================================================================
// Ultimates
//=============================================================================

bool Sweep::advance(Server &server, const Player &owner)
{
    if (delay > 0) {
        delay -= Server::TICK_DT;
        return true;
    }
    float to = std::min(start + range, front + speed * Server::TICK_DT);
    server.scorch(owner.getClientFd(), front, to, start - 1, top, bottom);
    front = to;
    return front < start + range;
}

// Johnny palms his gun: a wall of fire from floor to ceiling
void FireWave::fire(Server &, Player &, float start, float)
{
    waves.push_back({start, start, FIRE_WAVE_RANGE, FIRE_WAVE_SPEED, 0, 1 << 20});
}

void FireWave::act(Server &server, Player &player)
{
    std::erase_if(waves, [&](Sweep &wave) { return !wave.advance(server, player); });
}

void Nuke::fire(Server &, Player &, float start, float)
{
    shells.push_back({start + NUKE_DISTANCE, NUKE_DELAY});
}

// Razes everything around where it lands, from floor to ceiling
void Nuke::act(Server &server, Player &player)
{
    for (Shell &shell : shells) {
        if ((shell.timer -= Server::TICK_DT) <= 0)
            server.scorch(player.getClientFd(), shell.impact - NUKE_RADIUS, shell.impact + NUKE_RADIUS,
                          shell.impact - NUKE_RADIUS);
    }
    std::erase_if(shells, [](const Shell &shell) { return shell.timer <= 0; });
}

// Lucy's network breach, from just behind her to the end of the screen: drones go down, zappers go off
// for BREACH_TIME (the burnt ones stay out)
void Breach::fire(Server &server, Player &player, float start, float)
{
    for (int x = std::max(0, static_cast<int>(start - 2)); x <= static_cast<int>(start + BREACH_AHEAD); x++) {
        for (int y = 0; y < static_cast<int>(server.map().getHeight()); y++) {
            if (server.isLiveDrone(x, y))
                server.destroyDrone(player.getClientFd(), x, y);
            else if (server.map().getTile(x, y) == ZAPPER_TILE)
                server.switchOffZapper(x, y, BREACH_TIME);
        }
    }
}

void ThunderSpears::fire(Server &, Player &, float, float)
{
    give();
}

bool ThunderSpears::press(Server &server, Player &player)
{
    if (spears == 0)
        return false;
    if (throwSpear())
        server.fireProjectile(player, Weapon::Spear);
    return true;
}

bool ThunderSpears::throwSpear()
{
    if (spears == 0 || cooldown > 0)
        return false;
    spears--;
    cooldown = statsOf(Weapon::Spear).cooldown;
    return true;
}

// A big rocket at her height, razing a band of the corridor she can then fly through
void Fishbones::fire(Server &, Player &, float start, float line)
{
    auto [top, bottom] = fishbonesRows(line);
    rockets.push_back({start, start, FISHBONES_RANGE, FISHBONES_SPEED, top, bottom, FISHBONES_CHARGE_TIME});
    charge();
}

void Fishbones::act(Server &server, Player &player)
{
    std::erase_if(rockets, [&](Sweep &rocket) { return !rocket.advance(server, player); });
}

// The recoil starts once the charge is over
void Fishbones::tick(float dt)
{
    if (charge_timer > 0) {
        if ((charge_timer -= dt) <= 0)
            recoil_timer = FISHBONES_RECOIL_TIME;
        return;
    }
    recoil_timer = std::max(0.0f, recoil_timer - dt);
}

// Thrown back at FISHBONES_RECOIL_PUSH times the run speed, then picking her pace back up
float Fishbones::pace() const
{
    return 1 - (1 + FISHBONES_RECOIL_PUSH) * recoil_timer / FISHBONES_RECOIL_TIME;
}
