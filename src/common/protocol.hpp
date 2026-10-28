// Binary protocol: packets with a 4-byte header, reassembled from the TCP stream (see docs/PROTOCOL.md)

#ifndef PROTOCOL_HPP
    #define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

enum MessageType : uint8_t {
    MSG_CONNECT = 1,      // Client connecting
    MSG_MAP_DATA = 2,     // map columns, sent again as the run grows
    MSG_GAME_START = 3,   // game is starting, payload = your player number
    MSG_PLAYER_INPUT = 4, // jetpack on (1) or off (0)
    MSG_GAME_STATE = 5,   // positions, scores, etc.
    MSG_COLLISION = 6,    // something happened on a tile: a coin taken, a death, a drone down... (type, player, x, y)
    MSG_GAME_END = 7,     // game is over
    MSG_COUNTDOWN = 8,    // countdown before game starts, 0 = go
    MSG_READY = 9,        // client wants a rematch
    MSG_ATTACK = 10,      // CYBERPSYCHO: attack key pressed (payload 1, or none) or released (0)
    MSG_SHOT = 11,        // CYBERPSYCHO: someone attacked, payload = player number, weapon
    MSG_TITANS = 12,      // titans charging along the floor, every tick
    MSG_ULTIMATE = 13,    // CYBERPSYCHO: client fires its ultimate; server: someone did,
                          // payload = player number, ultimate, x where it starts (1/100 tile, 4 bytes),
                          // y of its line (1/100 tile, 2 bytes)
    MSG_SNIPERS = 14,     // Arasaka snipers aiming and firing, every tick
    MSG_BOSS = 15,        // Adam Smasher: where he is, his health, his attack, every tick
    MSG_VERSION = 16      // client: its PROTOCOL_VERSION, before MSG_CONNECT;
                          // server: not the same as mine, which is this (then it hangs up)
};

// Bumped whenever a packet changes: builds that don't speak the same one can't play together.
// Builds from before it existed send no MSG_VERSION: version 0.
constexpr uint8_t PROTOCOL_VERSION = 3;

struct MessageHeader {
    MessageType type;
    uint8_t payload_size[3];
};

// MSG_CONNECT: no district nor mode, when joining someone else's run
constexpr uint8_t NOT_PICKED = 0xFF;

// Positions travel in 1/100th of a tile
constexpr float POSITION_SCALE = 100.0f;

class Protocol {
public:
    static std::vector<uint8_t> createPacket(MessageType type, const std::vector<uint8_t> &payload);
    static bool extractPacket(std::vector<uint8_t> &stream, MessageHeader &header, std::vector<uint8_t> &payload);
    static uint32_t getPayloadSize(const MessageHeader &header);
    static void setPayloadSize(MessageHeader &header, uint32_t size);

    static void pushU16(std::vector<uint8_t> &data, uint16_t value);
    static uint16_t readU16(const uint8_t *data);
    static void pushU32(std::vector<uint8_t> &data, uint32_t value);
    static uint32_t readU32(const uint8_t *data);
};

#endif
