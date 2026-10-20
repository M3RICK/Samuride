// Ultimates (CYBERPSYCHO only): every eddy a runner earns charges a gauge, a full one fires their
// ultimate (the ultimate key, E by default). Shared by the server (who decides what they destroy) and
// the client (who draws them). Which runner has which ultimate: runners.hpp.
//
// The rule they all follow: none of them makes you invincible (the Sandevistan, the Attack Titan and
// the shield already do). They act on the scenery, you still steer and can die.

#ifndef ULTIMATES_HPP
    #define ULTIMATES_HPP

#include <cstdint>
#include <utility>

enum class Ultimate : uint8_t {
    None,
    FireWave,       // Johnny
    Nuke,           // David
    Breach,         // Lucy
    ThunderSpears,  // Mikasa
    Fishbones,      // Jinx
};

constexpr int ULTIMATE_CHARGE = 100;    // eddies to fill the gauge

// Johnny palms his gun: a wall of fire sweeps the screen ahead, burning drones, zappers and titans
constexpr float FIRE_WAVE_SPEED = 45;   // tiles per second, not on top of the run speed
constexpr float FIRE_WAVE_RANGE = 25;   // tiles from where it started

// David's arm cannon: a shell fired at the ground ahead, razing everything from floor to ceiling around
// where it lands. Measured from where it was fired: he runs into the blast, it doesn't spare him.
constexpr float NUKE_DELAY = 0.5f;      // seconds before it lands
constexpr float NUKE_DISTANCE = 10;     // tiles ahead of where it was fired
constexpr float NUKE_RADIUS = 8;        // tiles razed on each side of the impact

// Lucy's network breach: every drone on screen ahead goes down, every zapper there goes off a while
constexpr float BREACH_AHEAD = 15;      // tiles ahead of where it starts: the rest of the screen
constexpr float BREACH_TIME = 3.0f;

// Mikasa's thunder spears: thrown with the ultimate key, flying ahead until they stick in a drone or a
// titan (or reach their range), then blowing up in a lightning column from floor to ceiling
constexpr int THUNDER_SPEARS = 5;
constexpr float SPEAR_BLAST_RADIUS = 2;     // tiles on each side of the impact
constexpr char SPEAR_BLAST = 'l';           // collision type: a spear blew up at that column

// Jinx's Fishbones: a big rocket at her height, razing a band of the corridor she can then fly through
constexpr float FISHBONES_SPEED = 40;       // tiles per second, not on top of the run speed
constexpr float FISHBONES_RANGE = 30;       // tiles from where it was fired
constexpr float FISHBONES_HALF_HEIGHT = 2.5f;   // tiles razed above and below its line

// She stops dead and hangs in the air while it charges, then fires: its recoil throws her back, she
// runs backward at FISHBONES_RECOIL_PUSH times the run speed, then picks her pace back up over
// FISHBONES_RECOIL_TIME. Nothing protects her while she charges.
constexpr float FISHBONES_CHARGE_TIME = 0.6f;
constexpr float FISHBONES_RECOIL_TIME = 0.5f;
constexpr float FISHBONES_RECOIL_PUSH = 0.8f;

// Rows of the band Fishbones razes, around its line at y
inline std::pair<int, int> fishbonesRows(float y)
{
    return {static_cast<int>(y - FISHBONES_HALF_HEIGHT), static_cast<int>(y + FISHBONES_HALF_HEIGHT)};
}

#endif
