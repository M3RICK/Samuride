// A player on the server: physics, score, pickups and Attack Titan forms

#ifndef PLAYER_HPP
    #define PLAYER_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include "../common/shift.hpp"
#include "../common/powerups.hpp"
#include "../common/cyberpsycho.hpp"
#include "../common/upgrades.hpp"
#include "../common/difficulty.hpp"
#include "arsenal.hpp"

class Player {
private:
    int client_fd;
    int player_number = 0;

    // Position, in tiles
    float x = 0;
    float y = 0;

    // Velocity, in tiles per second
    float y_velocity = 0;

    // Game state
    int score = 0;
    bool jet_active = false;
    int character = 0;
    int version = 0;            // PROTOCOL_VERSION it sent, 0 if none
    bool introduced = false;    // sent MSG_CONNECT: the character is the one picked
    UpgradeLevels upgrades{};   // bought in the shop, sent with MSG_CONNECT
    float attack_cooldown = 0;  // seconds before the next CYBERPSYCHO attack
    Form form = Form::Normal;
    float form_timer = 0;       // seconds left in the current form
    bool shield = false;
    bool rescued = false;       // the Trauma Team came this run
    float rescue_timer = 0;     // seconds the Trauma Team is still at work on them
    float sprint_timer = 0;     // seconds of Sandevistan left
    float berserk_timer = 0;    // seconds of Berserk left
    float grace_timer = 0;      // seconds of invincibility left after a shield broke, a sprint or a titan
    int ultimate_charge = 0;    // eddies earned since the last ultimate, up to its runner's charge
    // The runner's weapon and ultimate, and what they keep (arsenal.hpp)
    std::unique_ptr<RunnerWeapon> runner_weapon = makeWeapon(weaponOf(0));
    std::unique_ptr<RunnerUltimate> runner_ultimate = makeUltimate(ultimateOf(0));
    std::set<std::pair<int, int>> collected_coins;

    // Bytes received but not yet forming a full packet, and bytes the socket couldn't take yet
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;

    void updateForm(float dt);

public:
    // The body is 1 tile wide and HEIGHT tiles tall, (x, y) is its top-left corner
    static constexpr float HEIGHT = 2.0f;

    explicit Player(int client_fd) : client_fd(client_fd) {}

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
    bool isGracing() const { return grace_timer > 0; }
    bool isSprinting() const { return sprint_timer > 0; }
    bool isBerserk() const { return berserk_timer > 0; }
    void powerUp(char tile);
    // Returns true when the shield took the hit
    bool absorbHit();
    // A deadly hit the shield didn't take: the Trauma Team picks them up, once per run. True if it came.
    bool callTraumaTeam();
    // While it works: on the floor, still, and true, the run waits
    bool awaitRescue(float dt, float max_y);
    void startShift();
    void addScore(int points);
    int getUltimateCharge() const { return ultimate_charge; }
    bool ultimateReady() const { return ultimate_charge >= ultimateCharge(character); }
    // The gauge, in percent of what this player needs
    int ultimatePercent() const { return ultimate_charge * 100 / ultimateCharge(character); }
    void spendUltimate() { ultimate_charge = 0; }
    RunnerWeapon &weapon() { return *runner_weapon; }
    const RunnerWeapon &weapon() const { return *runner_weapon; }
    RunnerUltimate &ultimate() { return *runner_ultimate; }
    const RunnerUltimate &ultimate() const { return *runner_ultimate; }

    bool canAttack() const { return attack_cooldown <= 0; }
    void startCooldown(float seconds) { attack_cooldown = seconds; }

    int getCharacter() const { return character; }
    void setCharacter(int id);
    void setUpgrades(const UpgradeLevels &levels) { upgrades = levels; }
    const UpgradeLevels &getUpgrades() const { return upgrades; }
    bool isIntroduced() const { return introduced; }
    void setVersion(int sent) { version = sent; }
    int getVersion() const { return version; }

    bool isJetActive() const { return jet_active; }
    void setJetActive(bool active) { jet_active = active; }

    std::vector<uint8_t> &getRecvBuffer() { return recv_buffer; }
    std::vector<uint8_t> &getSendBuffer() { return send_buffer; }

    void update(float dt, float forward_speed, float max_y);
};

#endif
