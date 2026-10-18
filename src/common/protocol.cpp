// Binary protocol: packets with a 4-byte header, reassembled from the TCP stream

#include "protocol.hpp"
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
    return packet;
}

// TCP is a stream: a recv() can hold half a packet or several of them.
// Pops one complete packet off the front of the stream, false if it isn't all there yet.
bool Protocol::extractPacket(std::vector<uint8_t> &stream, MessageHeader &header, std::vector<uint8_t> &payload)
{
    if (stream.size() < sizeof(MessageHeader))
        return false;

    memcpy(&header, stream.data(), sizeof(MessageHeader));
    size_t total = sizeof(MessageHeader) + getPayloadSize(header);

    if (stream.size() < total)
        return false;

    payload.assign(stream.begin() + sizeof(MessageHeader), stream.begin() + total);
    stream.erase(stream.begin(), stream.begin() + total);
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
    if (size > 0xFFFFFF) // Faut pas depasser 3 bytes sinn kaboom
        size = 0xFFFFFF;

    header.payload_size[0] = (size >> 16) & 0xFF;
    header.payload_size[1] = (size >> 8) & 0xFF;
    header.payload_size[2] = size & 0xFF;
}

void Protocol::pushU16(std::vector<uint8_t> &data, uint16_t value)
{
    data.push_back((value >> 8) & 0xFF);
    data.push_back(value & 0xFF);
}

uint16_t Protocol::readU16(const uint8_t *data)
{
    return (data[0] << 8) | data[1];
}

void Protocol::pushU32(std::vector<uint8_t> &data, uint32_t value)
{
    pushU16(data, value >> 16);
    pushU16(data, value & 0xFFFF);
}

uint32_t Protocol::readU32(const uint8_t *data)
{
    return (static_cast<uint32_t>(readU16(data)) << 16) | readU16(data + 2);
}
