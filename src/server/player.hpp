// A player on the server: physics, score, pickups and Attack Titan forms

#ifndef PLAYER_HPP
    #define PLAYER_HPP

#include <cstdint>
#include <set>
#include <utility>
#include <vector>
#include "../common/shift.hpp"

class Player {
private:
    int client_fd;
    int player_number;

    // Position, in tiles
    float x;
    float y;

    // Velocity, in tiles per second
    float y_velocity;

    // Game state
    int score;
    bool jet_active;
    int character;
    bool introduced;            // sent MSG_CONNECT: the character is the one picked
    float attack_cooldown;      // seconds before the next CYBERPSYCHO attack
    Form form;
    float form_timer;           // seconds left in the current form
    std::set<std::pair<int, int>> collected_coins;

    // Bytes received but not yet forming a full packet
    std::vector<uint8_t> recv_buffer;

    // Tuning knobs, in tiles and seconds. A modif si mal equilibré.
    static constexpr float GRAVITY = 28.0f;
    static constexpr float JET_POWER = -32.0f;
    static constexpr float MAX_VELOCITY = 10.0f;

    void updateForm(float dt);

public:
    // The body is 1 tile wide and HEIGHT tiles tall, (x, y) is its top-left corner
    static constexpr float HEIGHT = 2.0f;

    explicit Player(int client_fd);

    void reset(int number, float spawn_y);

    int getClientFd() const { return client_fd; }
    int getPlayerNumber() const { return player_number; }

    float getX() const { return x; }
    float getY() const { return y; }

    int getScore() const { return score; }
    bool collectCoin(int tile_x, int tile_y);
    bool takePickup(int tile_x, int tile_y);

    Form getForm() const { return form; }
    bool isInvincible() const { return form != Form::Normal; }
    void startShift();
    void addScore(int points) { score += points; }

    bool canAttack() const { return attack_cooldown <= 0; }
    void startCooldown(float seconds) { attack_cooldown = seconds; }

    int getCharacter() const { return character; }
    void setCharacter(int id) { character = id; introduced = true; }
    bool isIntroduced() const { return introduced; }

    bool isJetActive() const { return jet_active; }
    void setJetActive(bool active) { jet_active = active; }

    std::vector<uint8_t> &getRecvBuffer() { return recv_buffer; }

    void update(float dt, float forward_speed, float max_y);
};

#endif
