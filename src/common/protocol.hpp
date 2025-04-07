#ifndef PROTOCOL_HPP
    #define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

// Message types
enum MessageType : uint8_t {
    MSG_CONNECT = 1,      // Client connecting to server
    MSG_MAP_DATA = 2,     // Server sending map to client
    MSG_GAME_START = 3,   // Server notifying game is starting
    MSG_PLAYER_INPUT = 4, // Client sending input (space pressed/released)
    MSG_GAME_STATE = 5,   // Server sending positions, scores, etc.
    MSG_COLLISION = 6,    // Server notifying of coin/hazard collision
    MSG_GAME_END = 7      // Server notifying game is over
};

struct MessageHeader {
    MessageType type;      // 1 byte
    uint8_t payload_size[3]; // 3 bytes for payload size
};

class Protocol {
public:
    static std::vector<uint8_t> createPacket(MessageType type, const std::vector<uint8_t>& payload);
    static bool parseHeader(const char* data, size_t size, MessageHeader& header);
    static uint32_t getPayloadSize(const MessageHeader& header);
    static void setPayloadSize(MessageHeader& header, uint32_t size);
};

#endif
