// A player on the server: physics, score, pickups and Attack Titan forms

#include "player.hpp"
#include <algorithm>

Player::Player(int client_fd)
    : client_fd(client_fd), player_number(0), x(0), y(0), y_velocity(0),
      score(0), jet_active(false), character(0), introduced(false), upgrades{}, attack_cooldown(0), form(Form::Normal), form_timer(0),
      shield(false), sprint_timer(0), berserk_timer(0), grace_timer(0), ram(RAM_MAX), ram_timer(RAM_RECHARGE), ultimate_charge(0), spears(0), charge_timer(0), recoil_timer(0), burst_left(0), burst_timer(0) {
}

void Player::reset(int number, float spawn_y)
{
    player_number = number;
    x = 0;
    y = spawn_y;
    y_velocity = 0;
    score = 0;
    attack_cooldown = 0;
    form = Form::Normal;
    form_timer = 0;
    shield = upgrades[UPGRADE_SHIELD] > 0;
    sprint_timer = 0;
    berserk_timer = 0;
    grace_timer = 0;
    ram = RAM_MAX;
    ram_timer = RAM_RECHARGE;
    ultimate_charge = 0;
    spears = 0;
    charge_timer = 0;
    recoil_timer = 0;
    burst_left = 0;
    collected_coins.clear();
}

// Pickups stay on the map for the other player, each one only counts once per player
bool Player::takePickup(int tile_x, int tile_y)
{
    return collected_coins.insert({tile_x, tile_y}).second;
}

bool Player::collectCoin(int tile_x, int tile_y)
{
    if (!takePickup(tile_x, tile_y))
        return false;
    addScore(1);
    return true;
}

// Every eddy earned charges the ultimate
void Player::addScore(int points)
{
    score += points;
    ultimate_charge = std::min(ultimateCharge(upgrades), ultimate_charge + points);
}

void Player::startShift()
{
    form = Form::Shifting;
    form_timer = SHIFT_TIME;
}

void Player::powerUp(char tile)
{
    if (tile == SHIELD_TILE)
        shield = true;
    else if (tile == SANDEVISTAN_TILE)
        sprint_timer = sandevistanTime(upgrades);
    else if (tile == BERSERK_TILE)
        berserk_timer = BERSERK_TIME;
}

bool Player::absorbHit()
{
    if (!shield)
        return false;
    shield = false;
    grace_timer = SHIELD_GRACE;
    return true;
}

// Shifting -> Titan -> Emerging -> Normal; bursting out of the smoke throws you up
void Player::updateForm(float dt)
{
    if (form == Form::Normal)
        return;

    form_timer -= dt;
    if (form_timer > 0)
        return;

    if (form == Form::Shifting) {
        form = Form::Titan;
        form_timer = TITAN_TIME;
    } else if (form == Form::Titan) {
        form = Form::Emerging;
        form_timer = EMERGE_TIME;
        y_velocity = -MAX_VELOCITY;
    } else {
        form = Form::Normal;
        grace_timer = END_GRACE;
    }
}

void Player::update(float dt, float forward_speed, float max_y)
{
    updateForm(dt);
    attack_cooldown = std::max(0.0f, attack_cooldown - dt * (isBerserk() ? BERSERK_ATTACK_RATE : 1));
    grace_timer = std::max(0.0f, grace_timer - dt);
    if (ram < RAM_MAX && (ram_timer -= dt * (isBerserk() ? BERSERK_RAM_RATE : 1)) <= 0) {
        ram++;
        ram_timer = RAM_RECHARGE;
    }

    // Fishbones: standing still while it charges, the recoil once it fires
    bool charging = charge_timer > 0;
    if (charging && (charge_timer -= dt) <= 0)
        recoil_timer = FISHBONES_RECOIL_TIME;

    // Standing still while the smoke rises; the titan runs on the floor. Fishbones' recoil throws Jinx
    // backward, then she picks her pace back up.
    float pace = isSprinting() ? SANDEVISTAN_SPEED : 1;
    if (recoil_timer > 0 && !charging) {
        pace *= 1 - (1 + FISHBONES_RECOIL_PUSH) * recoil_timer / FISHBONES_RECOIL_TIME;
        recoil_timer = std::max(0.0f, recoil_timer - dt);
    }
    if (form != Form::Shifting && !charging)
        x += forward_speed * pace * dt;
    if (sprint_timer > 0 && (sprint_timer -= dt) <= 0) {
        sprint_timer = 0;
        grace_timer = END_GRACE;
    }
    berserk_timer = std::max(0.0f, berserk_timer - dt);
    if (form == Form::Titan) {
        y = max_y;
        y_velocity = 0;
        return;
    }
    // Hanging in the air, aiming
    if (charging) {
        y_velocity = 0;
        return;
    }

    bool thrust = jet_active && form != Form::Shifting;
    y_velocity += (thrust ? JET_POWER : GRAVITY) * dt;
    y_velocity = std::clamp(y_velocity, -MAX_VELOCITY, MAX_VELOCITY);
    y += y_velocity * dt;

    if (y < 0 || y > max_y) {
        y = std::clamp(y, 0.0f, max_y);
        y_velocity = 0;
    }
}

bool Player::burstShotDue(float dt)
{
    if (burst_left == 0 || (burst_timer -= dt) > 0)
        return false;
    burst_left--;
    burst_timer = MINIGUN_BURST_GAP;
    return true;
}
