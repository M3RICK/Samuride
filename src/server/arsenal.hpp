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
#include "../common/messages.hpp"

class Server;
class Player;

class RunnerWeapon {
public:
    virtual ~RunnerWeapon() = default;

    // The attack key, or Berserk attacking on its own, once the cooldown is over. Returns the cooldown
    // it starts, 0 when nothing happened.
    virtual float attack(Server &server, Player &player) = 0;
    // The attack key back up, same return: Wraith blinks then, as far as he held it
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
    // Berserk attacks with it on its own: not Wraith's Blink, he would teleport at random
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
    // True while the world stands still: Wraith's Stillness
    virtual bool freezes() const { return false; }
    // A deadly hit it takes instead of the runner (DirtyBastrd's chainsaw), from what `cause` (a MSG_COLLISION
    // type) says hit them at (x, y): true when it did
    virtual bool takesHit(Server &, Player &, char, int, int) { return false; }
    // A drone or a titan this runner took down, with anything
    virtual void killed() {}
    // DirtyBastrd's chainsaw, for MSG_GAME_STATE: out, lunging with it, the scrap eddies it would give now
    virtual bool sawOut() const { return false; }
    virtual bool lunging() const { return false; }
    virtual int scrap() const { return 0; }
    // What it has flying for the clients to draw, MSG_SAWS (DirtyBastrd's thrown saws; the server sets whose)
    virtual std::vector<SawState> inAir() const { return {}; }
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

// Wraith: BLINK_CHARGES teleports, killing the drones he crosses, stabbing Smasher if he reaches him
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

// DirtyBastrd: his Timberlands, the drones right in front of him (KICK_* in weapons.hpp); with his chainsaw out,
// a press of the key throws it instead, as it comes back up (Berserk never throws it, nor a press from before)
class Kick : public RunnerWeapon {
    // Steel Toes: a kicked drone in the air, the drone it flies into and the seconds before it gets there
    struct Flight {
        int to_x;
        int to_y;
        float left;
    };
    std::vector<Flight> flights;
    bool saw_out = false;   // his chainsaw is out: Berserk doesn't kick on its own then
    bool aimed = false;     // the key went down while it was out: its release throws it
    float out = 0;          // seconds his boot is still out, kicking what he runs into

    void strike(Server &server, Player &player);

    void launch(Server &server, Player &player, int x, int y, const std::vector<std::pair<int, int>> &kicked);

public:
    void act(Server &server, Player &player) override;
    float attack(Server &server, Player &player) override;
    float release(Server &server, Player &player) override;
    void tick(const Player &player, float dt) override;
    bool autoFires() const override { return !saw_out; }
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
// For STILLNESS_TIME the world stands still (Server::updatePlayers)
class Stillness : public RunnerUltimate {
    float timer = 0;        // seconds it still lasts

public:
    void fire(Server &, Player &, float, float) override { timer = STILLNESS_TIME; }
    void tick(float dt) override { timer = std::max(0.0f, timer - dt); }
    bool freezes() const override { return timer > 0; }
};

// DirtyBastrd's invisible chainsaw (CHAINSAW_* in ultimates.hpp): it shreds what's at his hands, lunges with the
// ultimate key again, and takes one hit before snapping into scrap eddies
class Chainsaw : public RunnerUltimate {
    float timer = 0;            // seconds it still runs; 0 once it ran out or snapped
    float held = 0;             // seconds it has been out, and what it took down: its scrap when it snaps
    int kills = 0;
    float lunge_timer = 0;      // seconds of the lunge left
    float lunge_cooldown = 0;   // seconds before the next lunge
    float zapper_cut_timer = 0; // seconds left of the lunge's window to cut through zappers
    std::vector<ThrownSaw> thrown;  // in the air, ricocheting or dropping

    void fly(Server &server, Player &player, ThrownSaw &saw);

public:
    void fire(Server &server, Player &player, float start, float line) override;
    bool press(Server &server, Player &player) override;
    void tick(float dt) override;
    void act(Server &server, Player &player) override;
    float pace() const override;
    bool takesHit(Server &server, Player &player, char cause, int x, int y) override;
    void killed() override;
    bool sawOut() const override { return timer > 0; }
    bool lunging() const override { return lunge_timer > 0; }
    int scrap() const override { return sawOut() ? chainsawScrap(kills, held) : 0; }
    void start();
    // True when he lunged: with the saw out, once the last lunge is far enough
    bool lunge();
    // The saw snaps: the scrap eddies it bursts into (the caller pays them), 0 without a saw
    int snap();
    // Lunging, it cuts the zapper he runs into: true when it did (a kill for its scrap), false when the
    // zapper kills him
    bool cutZapper();
    // It leaves his hands from where he holds it, with what it took down and how long he held it, and the
    // ultimate is over: false without a saw
    bool throwSaw(const Player &player);
    const std::vector<ThrownSaw> &flying() const { return thrown; }
    std::vector<SawState> inAir() const override;
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
