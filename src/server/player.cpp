#include "player.hpp"
#include <algorithm>

Player::Player(int client_fd)
    : client_fd(client_fd), player_number(0), x(0), y(0), y_velocity(0),
      score(0), jet_active(false) {
}

void Player::moveForward() {
    x += static_cast<int>(FORWARD_SPEED);
}

void Player::applyPhysics() {
    // Apply jetpack force if active
    if (jet_active) {
        y_velocity += JET_POWER;
    } else {
        // Apply gravity
        y_velocity += GRAVITY;
    }

    // Clamp velocity
    y_velocity = std::max(-MAX_VELOCITY, std::min(y_velocity, MAX_VELOCITY));

    // Update position
    y += static_cast<int>(y_velocity);
}
