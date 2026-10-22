// A player on the server: physics, score, pickups and Attack Titan forms

#ifndef PLAYER_HPP
    #define PLAYER_HPP

#include <algorithm>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>
#include "../common/shift.hpp"
#include "../common/powerups.hpp"
#include "../common/cyberpsycho.hpp"
#include "../common/upgrades.hpp"
#include "../common/difficulty.hpp"

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
    UpgradeLevels upgrades;     // bought in the shop, sent with MSG_CONNECT
    float attack_cooldown;      // seconds before the next CYBERPSYCHO attack
    Form form;
    float form_timer;           // seconds left in the current form
    bool shield;
    float sprint_timer;         // seconds of Sandevistan left
    float berserk_timer;        // seconds of Berserk left
    float grace_timer;          // seconds of invincibility left after a shield broke, a sprint or a titan
    int ram;                    // Lucy's hack charges
    float ram_timer;            // seconds before the next charge comes back
    int ultimate_charge;        // eddies earned since the last ultimate, up to its runner's charge
    int spears;                 // Mikasa's thunder spears left
    float spear_cooldown;       // seconds before she can throw the next one
    float charge_timer;         // seconds left charging Fishbones: standing still in the air
    float recoil_timer;         // seconds of Fishbones' recoil left
    int burst_left;             // Pow-Pow bullets still to come in this burst
    float burst_timer;          // seconds before the next one
    int quicksilver;            // the Hunter's bullets
    float trigger_time;         // seconds the attack key has been held, -1 while it's up (the Hunter's pistol)
    float rally_timer;          // seconds left to win the shield back (the Hunter's rally)
    std::set<std::pair<int, int>> collected_coins;

    // Bytes received but not yet forming a full packet, and bytes the socket couldn't take yet
    std::vector<uint8_t> recv_buffer;
    std::vector<uint8_t> send_buffer;

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
    bool isGracing() const { return grace_timer > 0; }
    bool isSprinting() const { return sprint_timer > 0; }
    bool isBerserk() const { return berserk_timer > 0; }
    void powerUp(char tile);
    // Returns true when the shield took the hit
    bool absorbHit();
    void startShift();
    void addScore(int points);
    int getUltimateCharge() const { return ultimate_charge; }
    bool ultimateReady() const { return ultimate_charge >= ultimateCharge(character); }
    // The gauge, in percent of what this player needs
    int ultimatePercent() const { return ultimate_charge * 100 / ultimateCharge(character); }
    void spendUltimate() { ultimate_charge = 0; }
    // Charges, then the recoil once it fires
    void startFishbones() { charge_timer = FISHBONES_CHARGE_TIME; }
    void startBurst(int bullets) { burst_left = bullets; burst_timer = MINIGUN_BURST_GAP; }
    // True when the next bullet of the burst is due this tick
    bool burstShotDue(float dt);
    int getSpears() const { return spears; }
    void giveSpears() { spears = THUNDER_SPEARS; }
    bool throwSpear();

    int getQuicksilver() const { return quicksilver; }
    void loadQuicksilver(int bullets) { quicksilver = std::min(QUICKSILVER_MAX, quicksilver + bullets); }
    void pressTrigger() { trigger_time = 0; }
    // Seconds it was held, -1 when it wasn't
    float releaseTrigger() { return std::exchange(trigger_time, -1.0f); }
    bool isAiming() const { return trigger_time >= PISTOL_HOLD && quicksilver > 0; }
    bool fireQuicksilver();
    bool isRallying() const { return rally_timer > 0; }
    // A kill during the rally: the shield is back
    bool rally();

    bool canAttack() const { return attack_cooldown <= 0; }
    void startCooldown(float seconds) { attack_cooldown = seconds; }
    int getRam() const { return ram; }
    void spendRam() { ram--; }

    int getCharacter() const { return character; }
    void setCharacter(int id) { character = id; introduced = true; }
    void setUpgrades(const UpgradeLevels &levels) { upgrades = levels; }
    const UpgradeLevels &getUpgrades() const { return upgrades; }
    bool isIntroduced() const { return introduced; }

    bool isJetActive() const { return jet_active; }
    void setJetActive(bool active) { jet_active = active; }

    std::vector<uint8_t> &getRecvBuffer() { return recv_buffer; }
    std::vector<uint8_t> &getSendBuffer() { return send_buffer; }

    void update(float dt, float forward_speed, float max_y);
};

#endif
