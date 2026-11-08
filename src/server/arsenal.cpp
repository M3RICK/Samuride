// The runners' weapons and ultimates on the server (see arsenal.hpp). The clients draw them in
// render/shots.cpp and render/ultimates.cpp.

#include "arsenal.hpp"
#include "server.hpp"
#include <cmath>

std::unique_ptr<RunnerWeapon> makeWeapon(Weapon id)
{
    switch (id) {
        case Weapon::Minigun:
            return std::make_unique<Minigun>();
        case Weapon::Blade:
            return std::make_unique<Blade>();
        case Weapon::Hack:
            return std::make_unique<Hack>();
        case Weapon::Rebellion:
            return std::make_unique<Rebellion>();
        case Weapon::Blink:
            return std::make_unique<Blink>();
        case Weapon::Kick:
            return std::make_unique<Kick>();
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
        case Ultimate::Doppelganger:
            return std::make_unique<Doppelganger>();
        case Ultimate::Stillness:
            return std::make_unique<Stillness>();
        case Ultimate::Chainsaw:
            return std::make_unique<Chainsaw>();
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
    // Double Tap: the second one flies through what the first took down
    if (player.hasBoon(Boon::Signature))
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
    if (player.hasBoon(Boon::Signature))
        burst_left += 2;    // Bigger Bursts
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
    if (player.hasBoon(Boon::Signature))
        stats.reach *= 1.5f;    // Long Edge

    server.announceShot(player, Weapon::Blade);
    server.strike(player, stats.reach);
    server.slashTitans(player, stats.reach);
    server.slashBoss(player, stats.reach);
    return stats.cooldown;
}

// Only when there is something in her way and RAM to hack it: a drone goes down, a zapper's beams go off,
// Adam Smasher takes a hit
float Hack::attack(Server &server, Player &player)
{
    auto [x, y] = hackTarget(server.map(), player.getX(), player.getY(), Player::HEIGHT,
                             [&server](int x, int y) { return server.isOff(x, y); });

    if (ram == 0)
        return 0;
    // Nothing in her way: Adam Smasher's cyberware, if he is there
    if (x < 0) {
        if (!server.hitBoss(player, BOSS_HACK_DAMAGE))
            return 0;
        ram--;
        server.announceShot(player, Weapon::Hack);
        return statsOf(Weapon::Hack).cooldown;
    }

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
    // Overclocked Deck: twice as fast
    float rate = (player.isBerserk() ? BERSERK_RAM_RATE : 1) * (player.hasBoon(Boon::Signature) ? 2 : 1);
    if (ram < RAM_MAX && (ram_timer -= dt * rate) <= 0) {
        ram++;
        ram_timer = RAM_RECHARGE;
    }
}

// The sword cuts like Mikasa's blade when a drone or a titan is within its reach, the guns shoot like
// Johnny's otherwise: the clients draw them as those. His Doppelganger repeats it from the opposite
// height (drones only: titans stay on the floor). The higher his rank, the faster he attacks.
float Rebellion::attack(Server &server, Player &player)
{
    WeaponStats sword = statsOf(Weapon::Blade);
    bool doubled = player.ultimate().doubles();
    float double_y = mirrored(player.getY(), Player::HEIGHT, server.map().getHeight());

    if (!server.inReach(player, sword.reach, player.getY())) {
        server.fireProjectile(player, Weapon::Gun);
        if (doubled)
            server.fireProjectile(player, Weapon::Gun, double_y);
        return statsOf(Weapon::Gun).cooldown * STYLE_HASTE[rank];
    }
    server.announceShot(player, Weapon::Blade);
    server.strike(player, sword.reach);
    server.slashTitans(player, sword.reach);
    server.slashBoss(player, sword.reach);
    if (doubled)
        server.strike(player, sword.reach, double_y);
    return sword.cooldown * STYLE_HASTE[rank];
}

// Paid at the rank he had; STYLE_KILLS of them raise it
int Rebellion::droneDown()
{
    int bounty = static_cast<int>(std::lround(DRONE_BOUNTY * STYLE_BOUNTY[rank]));

    if (++kills >= STYLE_KILLS && rank < STYLE_RANKS - 1) {
        rank++;
        kills = 0;
    }
    drop_timer = STYLE_DROP_TIME;
    return bounty;
}

// A rank down every STYLE_DROP_TIME without a kill
void Rebellion::tick(const Player &player, float dt)
{
    // Showoff: twice as slow
    if (rank == 0 || (drop_timer -= dt * (player.hasBoon(Boon::Signature) ? 0.5f : 1)) > 0)
        return;
    rank--;
    kills = 0;
    drop_timer = STYLE_DROP_TIME;
}

// The key down starts the aim; the Blink comes on release
float Blink::attack(Server &, Player &)
{
    if (charges_left > 0)
        held = 0;
    return 0;
}

// blinkDistance ahead, past a zapper there: the drones in the box his body sweeps go down and he picks up
// what's in it, a zapper on his way pays a close call, Smasher's rockets in it turn back, Smasher
// takes a stab if it reaches him
float Blink::release(Server &server, Player &player)
{
    if (held < 0 || charges_left == 0)
        return 0;
    float top = player.getY();
    float to_x = blinkLanding(player.getX() + (player.ultimate().freezes() ? BLINK_FAR_DISTANCE : blinkDistance(held)), top,
                              [&](int x, int y) { return server.isLiveZapper(x, y); });
    held = -1;
    float left = player.getX();
    float right = to_x + 1;
    float bottom = top + Player::HEIGHT;
    // Only a zapper that running would have hit: his body's rows (sampled like the collisions), the
    // columns between where he was and where he lands. Drones he cuts are just his attack, paid as kills.
    bool close_call = false;
    for (int x = static_cast<int>(left) + 1; x <= static_cast<int>(to_x); x++)
        for (float dy : {0.3f, 1.0f, 1.7f})
            close_call |= server.isLiveZapper(x, static_cast<int>(top + dy));

    for (int x = static_cast<int>(left); x < right; x++)
        for (int y = static_cast<int>(top); y < bottom; y++)
            if (server.isLiveDrone(x, y))
                server.destroyDrone(player.getClientFd(), x, y);
    if (close_call) {
        player.addScore(BLINK_CLOSE_CALL_BOUNTY);
        server.notifyCollision(CLOSE_CALL, player, static_cast<int>(to_x), static_cast<int>(top));
    }
    for (int x = static_cast<int>(left); x < right; x++)
        for (int y = static_cast<int>(top); y < bottom; y++)
            server.pickUp(player, x, y);
    if (server.bossBetween(left, right))
        server.hitBoss(player, BOSS_STAB_DAMAGE);
    // Smasher's rockets he passes through fly back at him
    if (int reflected = server.reflectRockets(left, right, top, bottom)) {
        server.hitBoss(player, reflected * BOSS_REFLECT_DAMAGE);
        player.dodge(BOSS_REFLECT_GRACE);
    }
    float strike = server.bossThreatens(player);
    // Before he moves: the clients draw his smoke where he was
    server.announceShot(player, Weapon::Blink);
    player.blinkTo(to_x);
    // Through the strike that was about to land on him, and a counter-stab wherever Smasher stands
    if (strike > 0) {
        player.dodge(strike);
        server.notifyCollision(PERFECT_DODGE, player, static_cast<int>(to_x), static_cast<int>(top));
        server.hitBoss(player, BOSS_STAB_DAMAGE);
        return BLINK_COOLDOWN;
    }
    // Stillness: the Blinks are free (and go all the way) while the world stands still
    if (player.ultimate().freezes())
        return BLINK_COOLDOWN;
    if (charges_left-- == BLINK_CHARGES)
        recharge_timer = BLINK_RECHARGE;
    return BLINK_COOLDOWN;
}

void Blink::tick(const Player &player, float dt)
{
    if (held >= 0)
        held += dt;
    // Void Walker: twice as fast
    float rate = (player.isBerserk() ? BERSERK_RAM_RATE : 1) * (player.hasBoon(Boon::Signature) ? 2 : 1);
    if (charges_left < BLINK_CHARGES && (recharge_timer -= dt * rate) <= 0) {
        charges_left++;
        recharge_timer = BLINK_RECHARGE;
    }
}

// Right after its own collision: the clients draw it from the last drone this player took to the one it flies into,
// or break it there when nothing is in range. Not into a drone this kick takes down, nor one another kicked drone
// already flies into.
void Kick::launch(Server &server, Player &player, int x, int y, const std::vector<std::pair<int, int>> &kicked)
{
    auto taken = [&](int tx, int ty) {
        return std::ranges::find(kicked, std::pair{tx, ty}) != kicked.end() ||
               std::ranges::any_of(flights, [&](const Flight &f) { return f.to_x == tx && f.to_y == ty; });
    };
    auto [to_x, to_y] = kickedDroneLands(x, y, [&](int tx, int ty) { return server.isLiveDrone(tx, ty) && !taken(tx, ty); });
    if (to_x < 0) {
        server.notifyCollision(DRONE_KICKED, player, x, y);
        return;
    }
    server.notifyCollision(DRONE_KICKED, player, to_x, to_y);
    flights.push_back({to_x, to_y, kickedDroneFlight(x, y, to_x, to_y)});
}

// The kicked drones get there: the one they fly into goes down, if nothing took it first
void Kick::act(Server &server, Player &player)
{
    for (Flight &flight : flights) {
        if ((flight.left -= Server::TICK_DT) <= 0 && server.isLiveDrone(flight.to_x, flight.to_y))
            server.destroyDrone(player.getClientFd(), flight.to_x, flight.to_y);
    }
    std::erase_if(flights, [](const Flight &flight) { return flight.left <= 0; });
}

// Every drone in reach goes down, no titan; Smasher takes it like a blade's cut when he's close. Steel Toes:
// each drone kicked down flies into the drone nearest to it (launch). Nothing while the saw is out: the key
// throws it as it comes back up (release).
float Kick::attack(Server &server, Player &player)
{
    if (player.ultimate().sawOut()) {
        aimed = true;
        return 0;
    }
    server.announceShot(player, Weapon::Kick);
    std::vector<std::pair<int, int>> kicked = server.dronesInReach(player, KICK_REACH, player.getY());
    for (auto [x, y] : kicked) {
        server.destroyDrone(player.getClientFd(), x, y);
        if (player.hasBoon(Boon::Signature))
            launch(server, player, x, y, kicked);
    }
    server.slashBoss(player, KICK_REACH);
    return KICK_COOLDOWN;
}

float Kick::release(Server &, Player &player)
{
    auto *saw = dynamic_cast<Chainsaw *>(&player.ultimate());
    bool throwing = std::exchange(aimed, false);
    return throwing && saw && saw->throwSaw(player) ? KICK_COOLDOWN : 0;
}

void Kick::tick(const Player &player, float)
{
    saw_out = player.ultimate().sawOut();
}

//=============================================================================
// Ultimates
//=============================================================================

bool Sweep::advance(Server &server, Player &owner)
{
    if (delay > 0) {
        delay -= Server::TICK_DT;
        return true;
    }
    float to = std::min(start + range, front + speed * Server::TICK_DT);
    server.scorch(owner.getClientFd(), front, to, start - 1, top, bottom);
    if (!hit_boss && server.bossBetween(start - 1, to))
        hit_boss = server.hitBoss(owner, BOSS_ULTIMATE_DAMAGE);
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
        if ((shell.timer -= Server::TICK_DT) > 0)
            continue;
        server.scorch(player.getClientFd(), shell.impact - NUKE_RADIUS, shell.impact + NUKE_RADIUS,
                      shell.impact - NUKE_RADIUS);
        // Smasher keeps pace while it falls: he's hit wherever he stands
        server.hitBoss(player, BOSS_ULTIMATE_DAMAGE);
    }
    std::erase_if(shells, [](const Shell &shell) { return shell.timer <= 0; });
}

// Lucy's network breach, from just behind her to the end of the screen: drones go down, zappers go off
// for BREACH_TIME (the burnt ones stay out), and her RAM comes back full
void Breach::fire(Server &server, Player &player, float start, float)
{
    if (auto *hack = dynamic_cast<Hack *>(&player.weapon()))
        hack->refill();
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

// He pulls it out of nowhere: it shreds in act, lunges with this same key (press)
void Chainsaw::fire(Server &, Player &, float, float)
{
    start();
}

void Chainsaw::start()
{
    timer = CHAINSAW_TIME;
    held = 0;
    kills = 0;
    lunge_cooldown = 0;
}

// While it runs, the key is the lunge's, ready or not
bool Chainsaw::press(Server &, Player &)
{
    if (!sawOut())
        return false;
    lunge();    // the clients see it in MSG_GAME_STATE
    return true;
}

bool Chainsaw::lunge()
{
    if (!sawOut() || lunge_cooldown > 0)
        return false;
    lunge_timer = CHAINSAW_LUNGE_TIME;
    lunge_cooldown = CHAINSAW_LUNGE_COOLDOWN;
    zapper_cut_timer = CHAINSAW_LUNGE_ZAPPER_TIME;
    return true;
}

// Running out gives nothing: only a snap pays
void Chainsaw::tick(float dt)
{
    lunge_timer = std::max(0.0f, lunge_timer - dt);
    lunge_cooldown = std::max(0.0f, lunge_cooldown - dt);
    zapper_cut_timer = std::max(0.0f, zapper_cut_timer - dt);
    if (!sawOut())
        return;
    held += dt;
    if ((timer -= dt) <= 0)
        timer = 0;
}

// Every drone in reach goes down, in the row at his hands only; the thrown saws fly on
void Chainsaw::act(Server &server, Player &player)
{
    for (ThrownSaw &saw : thrown)
        fly(server, player, saw);
    std::erase_if(thrown, [&](const ThrownSaw &saw) { return saw.landed(server.map().getHeight() - 0.5f); });
    if (!sawOut())
        return;
    int row = static_cast<int>(player.getY() + CHAINSAW_HEIGHT);
    float reach = lunging() ? CHAINSAW_LUNGE_REACH : CHAINSAW_REACH;
    for (int x = static_cast<int>(player.getX() + 0.5f); x <= static_cast<int>(player.getX() + 0.5f + reach); x++)
        if (server.isLiveDrone(x, row))
            server.destroyDrone(player.getClientFd(), x, row);
}

// Jinx's recoil the other way: thrown forward, then picking the run's pace back up
float Chainsaw::pace() const
{
    return 1 + CHAINSAW_LUNGE_PUSH * lunge_timer / CHAINSAW_LUNGE_TIME;
}

// A drone's, a titan's or Smasher's hit: it snaps, its scrap paid, a second of grace like a shield's. A titan
// that breaks it goes down with it. Lunging, a zapper's beams are cut and go out for good, the saw stays.
// Snipers go past it.
bool Chainsaw::takesHit(Server &server, Player &player, char cause, int x, int y)
{
    bool drone = cause == ZAPPER_TILE && server.isLiveDrone(x, y);
    if (cause == ZAPPER_TILE && !drone && cutZapper()) {
        for (std::pair<int, int> node : zapperNodes(server.map(), x, y))
            server.switchOffZapper(node.first, node.second, 1e9f);
        server.notifyCollision(CHAINSAW_ZAPPER_CUT, player, x, y);
        return true;
    }
    if (!sawOut() || !(drone || cause == TITAN_HIT || cause == BOSS_HIT))
        return false;
    if (cause == TITAN_HIT)
        server.sawTitans(player);
    int scrap = snap();
    player.addScore(scrap);
    player.dodge(SHIELD_GRACE);
    server.notifyCollision(CHAINSAW_SNAP, player, scrap, y);
    return true;
}

bool Chainsaw::cutZapper()
{
    if (!sawOut() || zapper_cut_timer <= 0)
        return false;
    kills++;
    return true;
}

void Chainsaw::killed()
{
    if (sawOut())
        kills++;
}

int Chainsaw::snap()
{
    int burst = scrap();
    timer = 0;
    held = 0;
    kills = 0;
    lunge_timer = 0;
    zapper_cut_timer = 0;
    return burst;
}

bool Chainsaw::throwSaw(const Player &player)
{
    if (!sawOut())
        return false;
    thrown.push_back({player.getX() + 1.3f, player.getY() + CHAINSAW_HEIGHT, kills, held});
    snap();
    return true;
}

// The thrown saw ricochets from what's ahead of him to what's next within its range, taking each down (his
// bounty, as if he had), then drops and pays its scrap where it lands, lying on the floor
void Chainsaw::fly(Server &server, Player &player, ThrownSaw &saw)
{
    int height = server.map().getHeight();
    float ahead_of = player.getX() + 0.5f;

    // What it can hit, and what each one is: a drone's or a zapper's tile, {-1, -1} a titan, {-2, -2} Smasher
    std::vector<SawTarget> targets;
    std::vector<std::pair<int, int>> tiles;
    std::vector<float> titan_x;
    int end = std::min(static_cast<int>(server.map().getWidth()) - 1, static_cast<int>(saw.x + CHAINSAW_THROW_RANGE));
    for (int x = std::max(0, static_cast<int>(std::max(ahead_of, saw.x - CHAINSAW_THROW_RANGE))); x <= end; x++) {
        for (int y = 0; y < height; y++) {
            if (server.isLiveDrone(x, y) || server.isLiveZapper(x, y)) {
                targets.push_back({x + 0.5f, y + 0.5f});
                tiles.push_back({x, y});
                titan_x.push_back(0);
            }
        }
    }
    for (const TitanState &titan : server.titanStates()) {
        TitanStats stats = titanStats(titan.type);
        targets.push_back({titan.x + stats.width / 2, height - stats.height / 2});
        tiles.push_back({-1, -1});
        titan_x.push_back(titan.x);
    }
    if (float boss = server.bossAt(); boss >= 0) {
        targets.push_back({boss + 1, height - 2.5f, true});
        tiles.push_back({-2, -2});
        titan_x.push_back(0);
    }

    int hit = saw.fly(Server::TICK_DT, targets, ahead_of);
    if (hit >= 0) {
        auto [x, y] = tiles[hit];
        server.notifyCollision(CHAINSAW_THROW_HIT, player, static_cast<int>(saw.x), static_cast<int>(saw.y));
        if (x == -2) {
            server.hitBoss(player, CHAINSAW_THROW_BOSS_DAMAGE);
            server.staggerBoss();
        } else if (x == -1) {
            server.takeDownTitan(player, titan_x[hit]);
        } else if (server.isLiveDrone(x, y)) {
            server.destroyDrone(player.getClientFd(), x, y);
        } else {
            for (std::pair<int, int> node : zapperNodes(server.map(), x, y))
                server.switchOffZapper(node.first, node.second, 1e9f);
            server.notifyCollision(CHAINSAW_ZAPPER_CUT, player, x, y);
        }
    }
    if (saw.landed(height - 0.5f)) {
        int scrap = saw.scrap();
        player.addScore(scrap);
        server.notifyCollision(CHAINSAW_THROW_SCRAP, player, static_cast<int>(saw.x), scrap);
    }
}

std::vector<SawState> Chainsaw::inAir() const
{
    std::vector<SawState> saws;
    for (const ThrownSaw &saw : thrown)
        saws.push_back({0, saw.x, saw.y, saw.dropping});
    return saws;
}
