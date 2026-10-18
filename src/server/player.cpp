// A player on the server: physics, score, pickups and Attack Titan forms

#include "player.hpp"
#include <algorithm>

Player::Player(int client_fd)
    : client_fd(client_fd), player_number(0), x(0), y(0), y_velocity(0),
      score(0), jet_active(false), character(0), attack_cooldown(0), form(Form::Normal), form_timer(0) {
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
    score++;
    return true;
}

void Player::startShift()
{
    form = Form::Shifting;
    form_timer = SHIFT_TIME;
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
    }
}

void Player::update(float dt, float forward_speed, float max_y)
{
    updateForm(dt);
    attack_cooldown = std::max(0.0f, attack_cooldown - dt);

    // Standing still while the smoke rises; the titan runs on the floor
    if (form != Form::Shifting)
        x += forward_speed * dt;
    if (form == Form::Titan) {
        y = max_y;
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
