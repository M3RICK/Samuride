// The runners' weapons and ultimates on the server: what each one does, and what it keeps between two
// ticks (Lucy's RAM, Mikasa's spears, a fire wave sweeping on...). Every player owns one of each, made
// from their runner's line in common/runners.hpp.
//
// A new weapon or ultimate: its id in common/weapons.hpp or common/ultimates.hpp (sent to the clients,
// who draw it), a class here, and its case in makeWeapon or makeUltimate.

#ifndef ARSENAL_HPP
    #define ARSENAL_HPP

#include <memory>
#include <vector>
#include "../common/weapons.hpp"
#include "../common/ultimates.hpp"
#include "../common/cyberpsycho.hpp"

class Server;
class Player;

class RunnerWeapon {
public:
    virtual ~RunnerWeapon() = default;

    // The attack key, or Berserk attacking on its own, once the cooldown is over. Returns the cooldown
    // it starts, 0 when nothing happened.
    virtual float attack(Server &server, Player &player) = 0;
    // The attack key back up, same return: Corvo blinks then, as far as he held it
    virtual float release(Server &, Player &) { return 0; }
    // Every tick, timers only (no server: the tests run it alone)
    virtual void tick(const Player &, float) {}
    // Every tick of a CYBERPSYCHO run, after the players moved
    virtual void act(Server &, Player &) {}
    // Shown over the runner's head (Lucy's RAM), sent in 2 bits
    virtual int charges() const { return 0; }
    // A drone this runner took down, with their weapon or their ultimate: returns its bounty
    virtual int droneDown() { return DRONE_BOUNTY; }
    // Their shield broke
    virtual void hurt() {}
    // Dante's style rank, sent in 3 bits
    virtual int style() const { return 0; }
    // Berserk attacks with it on its own: not Corvo's Blink, he would teleport at random
    virtual bool autoFires() const { return true; }
};

class RunnerUltimate {
public:
    virtual ~RunnerUltimate() = default;

    // Fired with a full gauge, from `start` (just ahead of the runner) along `line` (the middle of their body)
    virtual void fire(Server &server, Player &player, float start, float line) = 0;
    // The ultimate key while it's still going: true when it used it (Mikasa throws a spear)
    virtual bool press(Server &, Player &) { return false; }
    virtual void tick(float) {}
    virtual void act(Server &, Player &) {}
    // Shown over the runner's head and on the HUD (Mikasa's spears)
    virtual int charges() const { return 0; }
    // The runner's pace, times the run speed, and whether they hang still in the air: Fishbones
    virtual float pace() const { return 1; }
    virtual bool holdsStill() const { return false; }
    // True while a double repeats the runner's attacks: Dante's Doppelganger
    virtual bool doubles() const { return false; }
    // True when it hurts Smasher itself once it reaches him (Fishbones' rocket, the fire wave) or lands
    // (the nuke), instead of as it's fired
    virtual bool travels() const { return false; }
    // True while the world stands still: Corvo's Bend Time
    virtual bool freezes() const { return false; }
};

std::unique_ptr<RunnerWeapon> makeWeapon(Weapon id);
std::unique_ptr<RunnerUltimate> makeUltimate(Ultimate id);

// Johnny, David: a bullet
class Gun : public RunnerWeapon {
public:
    float attack(Server &server, Player &player) override;
};

// Jinx's Pow-Pow: bursts of MINIGUN_BURST bullets, a steady spray in Berserk
class Minigun : public RunnerWeapon {
    int burst_left = 0;     // bullets still to come in this burst
    float burst_timer = 0;  // seconds before the next one

public:
    float attack(Server &server, Player &player) override;
    void act(Server &server, Player &player) override;
    // The bullets after the first
    void startBurst() { burst_left = MINIGUN_BURST - 1; burst_timer = MINIGUN_BURST_GAP; }
    // True when the next bullet of the burst is due after dt
    bool burstShotDue(float dt);
};

// Mikasa: every drone and titan within reach goes down
class Blade : public RunnerWeapon {
public:
    float attack(Server &server, Player &player) override;
};

// Lucy: the first drone or zapper in her way, for one charge of RAM; a hacked drone passes it on
class Hack : public RunnerWeapon {
    int ram = RAM_MAX;
    float ram_timer = RAM_RECHARGE;     // seconds before the next charge comes back

    void bounce(Server &server, const Player &player, int from_x, int from_y);

public:
    float attack(Server &server, Player &player) override;
    void tick(const Player &player, float dt) override;
    int charges() const override { return ram; }
    void spendRam() { ram--; }
    void refill() { ram = RAM_MAX; ram_timer = RAM_RECHARGE; }
};

// Dante: his sword on the ground, Ebony & Ivory in the air, and his style meter (STYLE_* in weapons.hpp)
class Rebellion : public RunnerWeapon {
    int rank = 0;
    int kills = 0;              // in a row at this rank
    float drop_timer = 0;       // seconds before the rank drops

public:
    float attack(Server &server, Player &player) override;
    void tick(const Player &player, float dt) override;
    int droneDown() override;
    void hurt() override { rank = std::max(0, rank - STYLE_HIT_DROP); kills = 0; }
    int style() const override { return rank; }
};

// Corvo: BLINK_CHARGES teleports, killing the drones he crosses, stabbing Smasher if he reaches him
class Blink : public RunnerWeapon {
    int charges_left = BLINK_CHARGES;
    float recharge_timer = BLINK_RECHARGE;  // seconds before the next charge comes back
    float held = -1;                        // seconds the key has been down, -1 when it's up

public:
    float attack(Server &server, Player &player) override;
    float release(Server &server, Player &player) override;
    void tick(const Player &player, float dt) override;
    int charges() const override { return charges_left; }
    bool autoFires() const override { return false; }
};

// A wall razing the rows top to bottom as it sweeps ahead: Johnny's fire wave, Jinx's Fishbones
struct Sweep {
    float start;    // in tiles
    float front;
    float range;
    float speed;
    int top;
    int bottom;
    float delay = 0;    // seconds before it sets off: Fishbones charges first
    bool hit_boss = false;

    // Moves on by a tick; false once it reached its range
    bool advance(Server &server, Player &owner);
};

class FireWave : public RunnerUltimate {
    std::vector<Sweep> waves;

public:
    void fire(Server &server, Player &player, float start, float line) override;
    void act(Server &server, Player &player) override;
    bool travels() const override { return true; }
};

// David's nuke: it lands ahead after NUKE_DELAY
class Nuke : public RunnerUltimate {
    struct Shell {
        float impact;   // x where it lands, in tiles
        float timer;    // seconds before it does
    };
    std::vector<Shell> shells;

public:
    void fire(Server &server, Player &player, float start, float line) override;
    void act(Server &server, Player &player) override;
    bool travels() const override { return true; }
};

class Breach : public RunnerUltimate {
public:
    void fire(Server &server, Player &player, float start, float line) override;
};

// THUNDER_SPEARS, thrown one at a time with the ultimate key again (her blade keeps the attack key)
class ThunderSpears : public RunnerUltimate {
    int spears = 0;
    float cooldown = 0;     // seconds before she can throw the next one

public:
    void fire(Server &server, Player &player, float start, float line) override;
    bool press(Server &server, Player &player) override;
    void tick(float dt) override { cooldown = std::max(0.0f, cooldown - dt); }
    int charges() const override { return spears; }
    void give() { spears = THUNDER_SPEARS; }
    // One spear, if she has one and its cooldown is over
    bool throwSpear();
};

// His double at the opposite height, repeating his attacks (Rebellion::attack)
class Doppelganger : public RunnerUltimate {
    float timer = 0;        // seconds it still fights

public:
    void fire(Server &, Player &, float, float) override { timer = DOPPELGANGER_TIME; }
    void tick(float dt) override { timer = std::max(0.0f, timer - dt); }
    bool doubles() const override { return timer > 0; }
};

// She stops dead and hangs in the air while it charges, then its recoil throws her back
// For BEND_TIME the world stands still (Server::updatePlayers)
class BendTime : public RunnerUltimate {
    float timer = 0;        // seconds it still lasts

public:
    void fire(Server &, Player &, float, float) override { timer = BEND_TIME; }
    void tick(float dt) override { timer = std::max(0.0f, timer - dt); }
    bool freezes() const override { return timer > 0; }
};

class Fishbones : public RunnerUltimate {
    std::vector<Sweep> rockets;
    float charge_timer = 0;     // seconds left charging
    float recoil_timer = 0;     // seconds of recoil left

public:
    void fire(Server &server, Player &player, float start, float line) override;
    void act(Server &server, Player &player) override;
    bool travels() const override { return true; }
    void tick(float dt) override;
    float pace() const override;
    bool holdsStill() const override { return charge_timer > 0; }
    void charge() { charge_timer = FISHBONES_CHARGE_TIME; }
};

#endif
