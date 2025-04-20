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
      score(0), jet_active(false) {
}

// Tout les defines se retrouvent dans le hpp

void Player::moveForward()
{
    x += static_cast<int>(FORWARD_SPEED);
}

void Player::applyPhysics()
{
    if (jet_active) {
        y_velocity += JET_POWER;
    } else {
        y_velocity += GRAVITY;
    }

    y_velocity = std::max(-MAX_VELOCITY, std::min(y_velocity, MAX_VELOCITY));

    y += static_cast<int>(y_velocity);
}
