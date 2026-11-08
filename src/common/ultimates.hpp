// Ultimates (CYBERPSYCHO only): every eddy a runner earns charges a gauge, a full one fires their
// ultimate (the ultimate key, E by default). Shared by the server (who decides what they destroy) and
// the client (who draws them). Which runner has which ultimate: runners.hpp.
//
// The rule they all follow: none of them makes you invincible (the Sandevistan, the Attack Titan and
// the shield already do). They act on the scenery, you still steer and can die. DirtyBastrd's chainsaw
// bends it on purpose: it takes one hit, and only some.

#ifndef ULTIMATES_HPP
    #define ULTIMATES_HPP

#include <algorithm>
#include <cstdint>
#include <utility>

enum class Ultimate : uint8_t {
    None,
    FireWave,       // Johnny
    Nuke,           // David
    Breach,         // Lucy
    ThunderSpears,  // Mikasa
    Fishbones,      // Jinx
    Doppelganger,   // Dante
    Stillness,      // Wraith
    Chainsaw,       // DirtyBastrd
};

// Johnny palms his gun: a wall of fire sweeps the screen ahead, burning drones, zappers and titans
inline float FIRE_WAVE_SPEED = 45;   // tiles per second, not on top of the run speed
inline float FIRE_WAVE_RANGE = 25;   // tiles from where it started

// David's arm cannon: a shell fired at the ground ahead, razing everything from floor to ceiling around
// where it lands. Measured from where it was fired: he runs into the blast, it doesn't spare him.
inline float NUKE_DELAY = 0.5f;      // seconds before it lands
inline float NUKE_DISTANCE = 10;     // tiles ahead of where it was fired
inline float NUKE_RADIUS = 8;        // tiles razed on each side of the impact

// Lucy's network breach: every drone on screen ahead goes down, every zapper there goes off a while
inline float BREACH_AHEAD = 15;      // tiles ahead of where it starts: the rest of the screen
inline float BREACH_TIME = 3.0f;

// Mikasa's thunder spears: thrown with the ultimate key, flying ahead until they stick in a drone or a
// titan (or reach their range), then blowing up in a lightning column from floor to ceiling
inline int THUNDER_SPEARS = 5;
inline float SPEAR_BLAST_RADIUS = 2;     // tiles on each side of the impact
constexpr char SPEAR_BLAST = 'l';           // collision type: a spear blew up at that column

// Jinx's Fishbones: a big rocket at her height, razing a band of the corridor she can then fly through
inline float FISHBONES_SPEED = 40;       // tiles per second, not on top of the run speed
inline float FISHBONES_RANGE = 30;       // tiles from where it was fired
inline float FISHBONES_HALF_HEIGHT = 2.5f;   // tiles razed above and below its line

// She stops dead and hangs in the air while it charges, then fires: its recoil throws her back, she
// runs backward at FISHBONES_RECOIL_PUSH times the run speed, then picks her pace back up over
// FISHBONES_RECOIL_TIME. Nothing protects her while she charges.
inline float FISHBONES_CHARGE_TIME = 0.6f;
inline float FISHBONES_RECOIL_TIME = 0.5f;
inline float FISHBONES_RECOIL_PUSH = 0.8f;

// Dante's Doppelganger: for DOPPELGANGER_TIME his double, in red, flies at the opposite height
// (mirrored around the corridor's middle) and repeats every attack of his from there. It does nothing
// on its own. Its kills are his: they pay his style rank and raise it.
inline float DOPPELGANGER_TIME = 8;

// Wraith's Stillness: for STILLNESS_TIME the world stands still, only the runners move. Titans, sniper sights,
// Adam Smasher and his rockets stop where they are (still deadly to run into); Smasher no longer keeps
// pace, Wraith can catch up and blink into him.
inline float STILLNESS_TIME = 4;

// DirtyBastrd's invisible chainsaw: held out in front of him for CHAINSAW_TIME, shredding every drone
// within CHAINSAW_REACH each tick, in the row it is held at: drones at his head or his feet still get
// him, steering still matters. It takes one deadly hit from a drone, a titan or Smasher, then snaps (a titan
// that breaks it is cut down as it does); zappers and snipers still kill him.
// Like a shield, the snap leaves a second of grace (SHIELD_GRACE). Holding both, the saw goes first.
inline float CHAINSAW_TIME = 10.0f;
inline float CHAINSAW_REACH = 1.2f;      // tiles ahead: what is right in front of him, less than his kick
inline float CHAINSAW_HEIGHT = 1.0f;     // below the top of his body: his hands, where it cuts
constexpr char CHAINSAW_SNAP = 'n';         // collision type: the saw took a hit; x is the scrap it gave

// The ultimate key again while it runs: he revs and lunges, his pace pushed up by CHAINSAW_LUNGE_PUSH
// times the run speed and easing back over CHAINSAW_LUNGE_TIME (a tile and a half to three tiles further
// depending on the run speed, far less than the Sandevistan), the saw reaching CHAINSAW_LUNGE_REACH.
inline float CHAINSAW_LUNGE_TIME = 0.35f;
inline float CHAINSAW_LUNGE_PUSH = 1.0f;
inline float CHAINSAW_LUNGE_REACH = 2.5f;
inline float CHAINSAW_LUNGE_COOLDOWN = 1.2f;     // seconds from one lunge to the next

// When the saw snaps (not when it runs out), it bursts into eddies: the better it was used, the more.
// Kills are the drones and titans taken down while it was out, by the saw or by his kicks.
inline int CHAINSAW_SCRAP_BASE = 5;
inline int CHAINSAW_SCRAP_PER_KILL = 3;
inline int CHAINSAW_SCRAP_PER_SECOND = 1;    // for each second it lasted: kills should count most
inline int CHAINSAW_SCRAP_MAX = 30;          // a titan's bounty and two drones': a bonus, not a farm. Sent
                                             // in 5 bits: 31 at most

inline int chainsawScrap(int kills, float seconds)
{
    int scrap = CHAINSAW_SCRAP_BASE + CHAINSAW_SCRAP_PER_KILL * kills + static_cast<int>(CHAINSAW_SCRAP_PER_SECOND * seconds);
    return std::min(CHAINSAW_SCRAP_MAX, scrap);
}

// Where the double's body is, its top at y, for a body `height` tall in a corridor `floor` tiles high
inline float mirrored(float y, float height, float floor) { return floor - height - y; }

// Rows of the band Fishbones razes, around its line at y
inline std::pair<int, int> fishbonesRows(float y)
{
    return {static_cast<int>(y - FISHBONES_HALF_HEIGHT), static_cast<int>(y + FISHBONES_HALF_HEIGHT)};
}

#endif
