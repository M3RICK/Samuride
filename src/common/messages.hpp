// What the packets carry, and how each is written and read: the server and the client share these,
// so a field can't be written at one offset and read at another (see docs/PROTOCOL.md)

#ifndef MESSAGES_HPP
    #define MESSAGES_HPP

#include <cstdint>
#include <map>
#include <vector>
#include "protocol.hpp"
#include "cyberpsycho.hpp"
#include "titans.hpp"
#include "snipers.hpp"
#include "boss.hpp"
#include "shift.hpp"
#include "upgrades.hpp"

// MSG_CONNECT: district and mode are NOT_PICKED when joining someone else's run
struct ConnectRequest {
    int character = 0;
    uint8_t district = NOT_PICKED;
    uint8_t mode = NOT_PICKED;
    UpgradeLevels upgrades{};
};

// MSG_GAME_START: each client gets its own number, then who plays which runner
struct GameStart {
    int my_number = 0;
    std::map<int, int> roster;  // player number -> runner
    int district = 0;
    GameMode mode = GameMode::Classic;
};

// MSG_GAME_STATE, one per player
struct PlayerState {
    float x = 0;
    float y = 0;
    int score = 0;
    bool jet_active = false;
    Form form = Form::Normal;
    bool shielded = false;
    bool plated = false;        // a second shield over the first (Double Plating)
    bool sprinting = false;     // Sandevistan
    int ram = 0;                // the weapon's charges: Lucy's RAM, Wraith's Blinks
    bool berserk = false;
    int ultimate = 0;           // gauge, in percent
    int spears = 0;             // Mikasa's thunder spears left
    bool grace = false;         // invincible for a second after a shield, a sprint or a titan
    int style = 0;              // Dante's style rank, 0 (D) to 6 (SSS)
    int wanted = 0;             // NCPD wanted level, 0 to WANTED_MAX_STARS
    bool chainsaw = false;      // DirtyBastrd's saw is out
    bool lunging = false;       // and he lunges with it
    int scrap = 0;              // the eddies it would burst into if it snapped now
};

// MSG_SHOT: someone attacked in CYBERPSYCHO
struct ShotEvent {
    int player;
    Weapon weapon;
};

// MSG_ULTIMATE: someone fired their ultimate
struct UltimateEvent {
    int player;
    Ultimate ultimate;
    float x;    // where it starts, in tiles
    float y;    // the line it follows, for Fishbones
};

// MSG_TITANS, one per titan
struct TitanState {
    TitanType type;
    float x;    // left edge, in tiles
};

// MSG_SNIPERS, one per sniper
struct SniperState {
    SniperPhase phase;
    float x = 0;    // its bullet's nose, in tiles, once fired
    float y;        // its line, in tiles
};

// MSG_BOSS
struct BossState {
    bool fighting = false;
    int health = 100;       // percent
    BossAttack attack = BossAttack::None;
    BossPhase phase = BossPhase::Warning;
    float x = 0;            // where he stands, in tiles
    float rockets_x = 0;
    float y = 0;            // the rockets' gap top, or the cannon's line
    float time = 0;         // seconds into the phase
    bool sandevistan = false;   // attacking faster, low on health or late in his arena
    uint16_t reflected = 0;     // the rows of the rocket wall Wraith sent back at him, one bit each
};

// MSG_SAWS, one per thrown chainsaw in the air (DirtyBastrd)
struct SawState {
    int player;     // who threw it
    float x;        // its middle, in tiles
    float y;
    bool dropping;  // done ricocheting, falling to the floor
};

// MSG_COLLISION
struct CollisionEffect {
    char type;
    int player;
    int x;
    int y;
};

// Each encode has its decode; a decode returns false on a payload too short for what it reads
namespace Message {
    std::vector<uint8_t> encode(const ConnectRequest &connect);
    std::vector<uint8_t> encode(const GameStart &start);
    std::vector<uint8_t> encode(const std::map<int, PlayerState> &players);
    std::vector<uint8_t> encode(const ShotEvent &shot);
    std::vector<uint8_t> encode(const UltimateEvent &ultimate);
    std::vector<uint8_t> encode(const std::vector<TitanState> &titans);
    std::vector<uint8_t> encode(const std::vector<SniperState> &snipers);
    std::vector<uint8_t> encode(const BossState &boss);
    std::vector<uint8_t> encode(const CollisionEffect &collision);
    std::vector<uint8_t> encode(const std::vector<SawState> &saws);

    bool decode(const std::vector<uint8_t> &payload, ConnectRequest &connect);
    bool decode(const std::vector<uint8_t> &payload, GameStart &start);
    bool decode(const std::vector<uint8_t> &payload, std::map<int, PlayerState> &players);
    bool decode(const std::vector<uint8_t> &payload, ShotEvent &shot);
    bool decode(const std::vector<uint8_t> &payload, UltimateEvent &ultimate);
    bool decode(const std::vector<uint8_t> &payload, std::vector<TitanState> &titans);
    bool decode(const std::vector<uint8_t> &payload, std::vector<SniperState> &snipers);
    bool decode(const std::vector<uint8_t> &payload, BossState &boss);
    bool decode(const std::vector<uint8_t> &payload, CollisionEffect &collision);
    bool decode(const std::vector<uint8_t> &payload, std::vector<SawState> &saws);
}

#endif
