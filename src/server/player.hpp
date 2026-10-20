// A player on the server: physics, score, pickups and Attack Titan forms

#ifndef PLAYER_HPP
    #define PLAYER_HPP

#include <cstdint>
#include <set>
#include <utility>
#include <vector>
#include "../common/shift.hpp"
#include "../common/powerups.hpp"
#include "../common/cyberpsycho.hpp"

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
    bool shield;
    float sprint_timer;         // seconds of Sandevistan left
    float berserk_timer;        // seconds of Berserk left
    float grace_timer;          // seconds of invincibility left after a shield broke
    int ram;                    // Lucy's hack charges
    float ram_timer;            // seconds before the next charge comes back
    int ultimate_charge;        // eddies earned since the last ultimate, up to ULTIMATE_CHARGE
    int spears;                 // Mikasa's thunder spears left
    bool attack_held;
    std::set<std::pair<int, int>> collected_coins;

    // Bytes received but not yet forming a full packet, and bytes the socket couldn't take yet
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;

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
    bool isInvincible() const { return form != Form::Normal || sprint_timer > 0 || grace_timer > 0; }
    bool hasShield() const { return shield; }
    bool isSprinting() const { return sprint_timer > 0; }
    bool isBerserk() const { return berserk_timer > 0; }
    void powerUp(char tile);
    // Returns true when the shield took the hit
    bool absorbHit();
    void startShift();
    void addScore(int points);
    int getUltimateCharge() const { return ultimate_charge; }
    bool ultimateReady() const { return ultimate_charge >= ULTIMATE_CHARGE; }
    void spendUltimate() { ultimate_charge = 0; }
    bool isAttackHeld() const { return attack_held; }
    void setAttackHeld(bool held) { attack_held = held; }
    int getSpears() const { return spears; }
    void giveSpears() { spears = THUNDER_SPEARS; }
    void useSpear() { spears--; }

    bool canAttack() const { return attack_cooldown <= 0; }
    void startCooldown(float seconds) { attack_cooldown = seconds; }
    int getRam() const { return ram; }
    void spendRam() { ram--; }

    int getCharacter() const { return character; }
    void setCharacter(int id) { character = id; introduced = true; }
    bool isIntroduced() const { return introduced; }

    bool isJetActive() const { return jet_active; }
    void setJetActive(bool active) { jet_active = active; }

    std::vector<uint8_t> &getRecvBuffer() { return recv_buffer; }
    std::vector<uint8_t> &getSendBuffer() { return send_buffer; }

    void update(float dt, float forward_speed, float max_y);
};

#endif
