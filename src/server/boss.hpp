// Adam Smasher on the server (common/boss.hpp has his numbers): he wakes up at the arena, keeps pace
// ahead of the leader, attacks in turn and leaves at its end, unless shot down first. Streamed with MSG_BOSS.

#ifndef SERVER_BOSS_HPP
    #define SERVER_BOSS_HPP

#include <cstdint>
#include <map>
#include <random>
#include <vector>
#include "../common/boss.hpp"

class Player;

class Boss {
public:
    void reset();
    void update(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players, float floor);
    // In the arena and still standing
    bool fighting() const { return stage == Stage::Fighting; }
    // Where he stands, in tiles: shots reaching it hit him. Close by after a slam.
    float x() const;
    bool hit(const Player &player, float floor) const;
    // Takes `amount` off his health; true when it downed him
    bool damage(int amount);

    // [fighting][health %][attack][phase][his x, 4 bytes][the rockets' x, 4 bytes][y, 2 bytes][seconds into the phase, 2 bytes]
    std::vector<uint8_t> serialize() const;

    // The attack and its phase `time` seconds into it, None once it's over
    static BossPhase phaseOf(BossAttack attack, float time);
    static float durationOf(BossAttack attack);

private:
    enum class Stage { Waiting, Fighting, Gone };

    Stage stage = Stage::Waiting;
    float leader = 0;           // the leader's x, last tick
    int health = 0;
    BossAttack attack = BossAttack::None;
    float time = 0;             // seconds into the attack, or before the next one (negative)
    float y = 0;                // the rockets' gap top, or the cannon's line
    int target_fd = -1;         // whom the cannon follows

    void start(std::mt19937 &rng, const std::map<int, Player> &players, float floor);
    // The rockets' x, in tiles
    float rocketsX() const;
};

#endif
