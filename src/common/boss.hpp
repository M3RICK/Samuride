// Adam Smasher, the first boss: at BOSS_DISTANCE he steps in on the right of the screen and keeps pace
// with the runners while the map clears around him (the arena), then attacks in turn, each attack
// warned first. CLASSIC: survive to the end of the arena. CYBERPSYCHO: shoot him down for a bounty.
// Shared by the server (who attacks and checks hits) and the client (who draws him).

#ifndef BOSS_HPP
    #define BOSS_HPP

#include <cstdint>

// The arena: from BOSS_DISTANCE, BOSS_ARENA tiles with nothing but him (about 30 s at that speed)
inline float BOSS_DISTANCE = 500;
inline float BOSS_ARENA = 400;
// He stands this many tiles ahead of the leader: the right edge of the screen
inline float BOSS_AHEAD = 13;
// Seconds of his entrance before the first attack, then between two attacks
inline float BOSS_INTRO_TIME = 3;
inline float BOSS_ATTACK_GAP = 1.2f;

// CYBERPSYCHO: his health, what each hit takes, and the bounty for the one who downs him
inline int BOSS_HEALTH = 80;
inline int BOSS_SHOT_DAMAGE = 1;        // a bullet: Johnny, David, Jinx, Dante
inline int BOSS_SPEAR_DAMAGE = 8;       // one of Mikasa's thunder spears
inline int BOSS_HACK_DAMAGE = 8;        // Lucy hacking his cyberware, for a charge of RAM
inline int BOSS_ULTIMATE_DAMAGE = 15;   // any ultimate fired while he is there
inline int BOSS_BOUNTY = 150;

// Rockets: a wall of them along every row but a gap BOSS_ROCKET_GAP tall, warned on the right edge,
// then flying at the runners
inline float BOSS_ROCKET_WARN = 1.2f;
inline float BOSS_ROCKET_SPEED = 22;    // tiles per second, toward the runners
inline float BOSS_ROCKET_GAP = 4;
// Ground slam: the floor glows, then a shockwave runs along it, BOSS_SLAM_HEIGHT tiles tall: be in the air
inline float BOSS_SLAM_WARN = 1.0f;
inline float BOSS_SLAM_TIME = 0.5f;
inline float BOSS_SLAM_HEIGHT = 3;
// Arm cannon: like a sniper, his sight follows a runner, freezes, fires along the row
inline float BOSS_CANNON_TRACK = 1.2f;
inline float BOSS_CANNON_AIM = 0.5f;
inline float BOSS_CANNON_TIME = 0.3f;
inline float BOSS_CANNON_HALF = 0.6f;   // half the beam's height

enum class BossAttack : uint8_t { None, Rockets, Slam, Cannon };
// Warning: the threat is shown, harmless. Strike: it hurts. Aim: the cannon frozen before it fires.
enum class BossPhase : uint8_t { Warning, Aim, Strike };

// MSG_COLLISION types: he hit a player; he went down (x where he stood)
constexpr char BOSS_HIT = 'm';
constexpr char BOSS_KILL = 'K';

inline bool inBossArena(float x)
{
    return x >= BOSS_DISTANCE && x < BOSS_DISTANCE + BOSS_ARENA;
}

// A body whose top is at py (2 tiles tall, 0.2 of margin at each end) against a band from top to bottom
inline bool bandHits(float top, float bottom, float py)
{
    return top < py + 1.8f && bottom > py + 0.2f;
}

// The rows a rocket flies along (their middle), every row but the gap from gap_top
inline bool rocketRow(int row, float gap_top)
{
    return row + 0.5f < gap_top || row + 0.5f > gap_top + BOSS_ROCKET_GAP;
}

// A rocket wall `ahead` tiles in front of a runner whose top is at py, on a floor `floor` tall
inline bool rocketsHit(float ahead, float gap_top, float py, int floor)
{
    if (ahead > 1 || ahead < -0.5f)
        return false;
    for (int row = 0; row < floor; row++)
        if (rocketRow(row, gap_top) && bandHits(row + 0.15f, row + 0.85f, py))
            return true;
    return false;
}

#endif
