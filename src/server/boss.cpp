// Adam Smasher on the server (see boss.hpp)

#include "boss.hpp"
#include "player.hpp"
#include "../common/protocol.hpp"
#include <algorithm>

void Boss::reset()
{
    stage = Stage::Waiting;
    health = BOSS_HEALTH;
    attack = BossAttack::None;
    time = 0;
}

float Boss::durationOf(BossAttack attack)
{
    switch (attack) {
        case BossAttack::Rockets:
            // Until the wall is well past the runners
            return BOSS_ROCKET_WARN + (BOSS_AHEAD + 3) / BOSS_ROCKET_SPEED;
        case BossAttack::Slam:
            return BOSS_SLAM_WARN + BOSS_SLAM_TIME;
        case BossAttack::Cannon:
            return BOSS_CANNON_TRACK + BOSS_CANNON_AIM + BOSS_CANNON_TIME;
        default:
            return 0;
    }
}

BossPhase Boss::phaseOf(BossAttack attack, float time)
{
    switch (attack) {
        case BossAttack::Rockets:
            return time < BOSS_ROCKET_WARN ? BossPhase::Warning : BossPhase::Strike;
        case BossAttack::Slam:
            return time < BOSS_SLAM_WARN ? BossPhase::Warning : BossPhase::Strike;
        case BossAttack::Cannon:
            if (time < BOSS_CANNON_TRACK)
                return BossPhase::Warning;
            return time < BOSS_CANNON_TRACK + BOSS_CANNON_AIM ? BossPhase::Aim : BossPhase::Strike;
        default:
            return BossPhase::Warning;
    }
}

float Boss::rocketsX() const
{
    float flying = std::max(0.0f, time - BOSS_ROCKET_WARN);
    return x() - flying * BOSS_ROCKET_SPEED;
}

void Boss::update(float leader_x, float dt, std::mt19937 &rng, const std::map<int, Player> &players, float floor)
{
    leader = leader_x;
    if (stage == Stage::Waiting && inBossArena(leader_x)) {
        stage = Stage::Fighting;
        time = -BOSS_INTRO_TIME;
    }
    if (stage != Stage::Fighting)
        return;
    if (leader_x >= BOSS_DISTANCE + BOSS_ARENA) {
        stage = Stage::Gone;
        return;
    }

    time += dt;
    if (attack == BossAttack::None && time >= 0)
        start(rng, players, floor);
    if (attack == BossAttack::Cannon && phaseOf(attack, time) == BossPhase::Warning) {
        auto target = players.find(target_fd);
        if (target != players.end())
            y = target->second.getY() + Player::HEIGHT / 2;
    }
    if (attack != BossAttack::None && time >= durationOf(attack)) {
        attack = BossAttack::None;
        time = -BOSS_ATTACK_GAP;
    }
}

// Any attack but the last one, aimed at a random runner
void Boss::start(std::mt19937 &rng, const std::map<int, Player> &players, float floor)
{
    BossAttack last = attack;
    do
        attack = static_cast<BossAttack>(std::uniform_int_distribution<int>(1, 3)(rng));
    while (attack == last);
    time = 0;

    if (attack == BossAttack::Rockets)
        y = std::uniform_real_distribution<float>(0, floor - BOSS_ROCKET_GAP)(rng);
    if (attack == BossAttack::Cannon && !players.empty()) {
        auto target = std::next(players.begin(), std::uniform_int_distribution<size_t>(0, players.size() - 1)(rng));
        target_fd = target->first;
        y = target->second.getY() + Player::HEIGHT / 2;
    }
}

bool Boss::hit(const Player &player, float floor) const
{
    if (!fighting() || attack == BossAttack::None || phaseOf(attack, time) != BossPhase::Strike)
        return false;
    switch (attack) {
        case BossAttack::Rockets:
            return rocketsHit(rocketsX() - player.getX(), y, player.getY(), static_cast<int>(floor));
        case BossAttack::Slam:
            return bandHits(floor - BOSS_SLAM_HEIGHT, floor, player.getY());
        default:
            return bandHits(y - BOSS_CANNON_HALF, y + BOSS_CANNON_HALF, player.getY());
    }
}

bool Boss::damage(int amount)
{
    if (!fighting())
        return false;
    health -= amount;
    if (health > 0)
        return false;
    stage = Stage::Gone;
    return true;
}

std::vector<uint8_t> Boss::serialize() const
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(fighting()),
                                 static_cast<uint8_t>(std::clamp(health * 100 / std::max(1, BOSS_HEALTH), 0, 100)),
                                 static_cast<uint8_t>(attack), static_cast<uint8_t>(phaseOf(attack, time))};

    Protocol::pushU32(data, x() * POSITION_SCALE);
    Protocol::pushU32(data, std::max(0.0f, rocketsX()) * POSITION_SCALE);
    Protocol::pushU16(data, std::max(0.0f, y) * POSITION_SCALE);
    Protocol::pushU16(data, std::max(0.0f, time) * POSITION_SCALE);
    return data;
}
