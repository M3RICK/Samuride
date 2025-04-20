/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "protocol.hpp"
#include "debug.hpp"
#include <cstring>

std::vector<uint8_t> Protocol::createPacket(MessageType type, const std::vector<uint8_t> &payload)
{
    MessageHeader header;
    header.type = type;
    setPayloadSize(header, payload.size());

    std::vector<uint8_t> packet(sizeof(MessageHeader) + payload.size());

    memcpy(packet.data(), &header, sizeof(MessageHeader));

    if (!payload.empty()) {
        memcpy(packet.data() + sizeof(MessageHeader), payload.data(), payload.size());
    }

    DEBUG_LOG("Created packet: Type=" + std::to_string(type) +
              ", Size=" + std::to_string(payload.size()));

    return packet;
}

bool Protocol::parseHeader(const char *data, size_t size, MessageHeader &header)
{
    if (size < sizeof(MessageHeader)) {
        DEBUG_LOG("Error parsing header: insufficient data");
        return false;
    }

    memcpy(&header, data, sizeof(MessageHeader));

    DEBUG_LOG("Parsed header: Type=" + std::to_string(header.type) +
              ", Size=" + std::to_string(getPayloadSize(header)));

    return true;
}

uint32_t Protocol::getPayloadSize(const MessageHeader &header)
{
    return (header.payload_size[0] << 16) |
           (header.payload_size[1] << 8) |
           header.payload_size[2];
}

void Protocol::setPayloadSize(MessageHeader &header, uint32_t size)
{
    if (size > 0xFFFFFF) { // Faut pas depasser 3 bytes sinn kaboom
        DEBUG_LOG("Warning: Payload size too large, truncating");
        size = 0xFFFFFF;
    }

    header.payload_size[0] = (size >> 16) & 0xFF;
    header.payload_size[1] = (size >> 8) & 0xFF;
    header.payload_size[2] = size & 0xFF;
}
