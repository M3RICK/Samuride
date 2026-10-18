/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef PROTOCOL_HPP
    #define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

enum MessageType : uint8_t {
    MSG_CONNECT = 1,      // Client connecting
    MSG_MAP_DATA = 2,     // map columns, sent again as the run grows
    MSG_GAME_START = 3,   // game is starting, payload = your player number
    MSG_PLAYER_INPUT = 4, // input (space pressed/released)
    MSG_GAME_STATE = 5,   // positions, scores, etc.
    MSG_COLLISION = 6,    // collision
    MSG_GAME_END = 7,     // game is over
    MSG_COUNTDOWN = 8,    // countdown before game starts, 0 = go
    MSG_READY = 9,        // client wants a rematch
    MSG_ATTACK = 10,      // CYBERPSYCHO: client attacks
    MSG_SHOT = 11         // CYBERPSYCHO: someone attacked, payload = player number, weapon
};

struct MessageHeader {
    MessageType type;
    uint8_t payload_size[3];
};

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
