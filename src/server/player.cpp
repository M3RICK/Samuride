/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "player.hpp"
#include <algorithm>

Player::Player(int client_fd)
    : client_fd(client_fd), player_number(0), x(0), y(0), y_velocity(0),
      score(0), jet_active(false), character(0) {
}

void Player::reset(int number, float spawn_y)
{
    player_number = number;
    x = 0;
    y = spawn_y;
    y_velocity = 0;
    score = 0;
    collected_coins.clear();
}

// Coins stay on the map for the other player, each one only counts once per player
bool Player::collectCoin(int tile_x, int tile_y)
{
    if (!collected_coins.insert({tile_x, tile_y}).second)
        return false;
    score++;
    return true;
}

void Player::update(float dt, float forward_speed, float max_y)
{
    x += forward_speed * dt;

    y_velocity += (jet_active ? JET_POWER : GRAVITY) * dt;
    y_velocity = std::clamp(y_velocity, -MAX_VELOCITY, MAX_VELOCITY);
    y += y_velocity * dt;

    if (y < 0 || y > max_y) {
        y = std::clamp(y, 0.0f, max_y);
        y_velocity = 0;
    }
}
