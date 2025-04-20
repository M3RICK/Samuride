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
    MSG_MAP_DATA = 2,     // sending map to client
    MSG_GAME_START = 3,   // game is starting
    MSG_PLAYER_INPUT = 4, // input (space pressed/released)
    MSG_GAME_STATE = 5,   // positions, scores, etc.
    MSG_COLLISION = 6,    // collision
    MSG_GAME_END = 7,      // game is over
    MSG_COUNTDOWN = 8     // countdown before game starts
};

struct MessageHeader {
    MessageType type;
    uint8_t payload_size[3];
};

class Protocol {
public:
    static std::vector<uint8_t> createPacket(MessageType type, const std::vector<uint8_t> &payload);
    static bool parseHeader(const char *data, size_t size, MessageHeader &header);
    static uint32_t getPayloadSize(const MessageHeader &header);
    static void setPayloadSize(MessageHeader &header, uint32_t size);
};

#endif
