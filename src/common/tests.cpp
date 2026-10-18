/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "map.hpp"
#include "protocol.hpp"
#include "../server/generator.hpp"
#include <cassert>
#include <iostream>

static void testPacketFraming()
{
    auto a = Protocol::createPacket(MSG_COUNTDOWN, {3});
    auto b = Protocol::createPacket(MSG_GAME_END, {0xFF});
    std::vector<uint8_t> stream(a);
    stream.insert(stream.end(), b.begin(), b.begin() + 2);

    MessageHeader header;
    std::vector<uint8_t> payload;

    // Two packets glued together, the second one cut in half
    assert(Protocol::extractPacket(stream, header, payload));
    assert(header.type == MSG_COUNTDOWN && payload == std::vector<uint8_t>{3});
    assert(!Protocol::extractPacket(stream, header, payload));

    stream.insert(stream.end(), b.begin() + 2, b.end());
    assert(Protocol::extractPacket(stream, header, payload));
    assert(header.type == MSG_GAME_END && payload == std::vector<uint8_t>{0xFF});
    assert(stream.empty());
}

static void testIntegers()
{
    std::vector<uint8_t> data;
    Protocol::pushU16(data, 8700);
    Protocol::pushU32(data, 4000000000u);
    assert(Protocol::readU16(data.data()) == 8700);
    assert(Protocol::readU32(data.data() + 2) == 4000000000u);
}

static void assertSameMap(const Map &a, const Map &b)
{
    assert(a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight());
    for (size_t y = 0; y < a.getHeight(); y++)
        for (size_t x = 0; x < a.getWidth(); x++)
            assert(a.getTile(x, y) == b.getTile(x, y));
}

static void testMapChunks()
{
    Map server;
    assert(server.loadFromFile("maps/small_good.txt"));

    // Client gets the whole map, then a chunk appended by the server
    Map client;
    assert(client.applyData(server.serialize()));
    size_t from = server.getWidth();
    assert(server.appendChunk(std::vector<std::string>(server.getHeight(), "c_e")));
    assert(client.applyData(server.serialize(from)));
    assertSameMap(server, client);

    // A chunk that doesn't continue the map is refused, a new map (offset 0) replaces it
    assert(!client.applyData(server.serialize(1)));
    Map small;
    assert(small.appendChunk({"__", "ce"}));
    assert(client.applyData(small.serialize()));
    assertSameMap(small, client);
}

static void testGenerator()
{
    std::mt19937 rng(42);

    for (int run = 0; run < 200; run++) {
        float difficulty = run / 199.0f;
        auto rows = generateChunk(rng, 40, 10, difficulty);
        assert(rows.size() == 10);

        for (size_t x = 0; x < 40; x++) {
            int zappers = 0;
            for (const auto &row : rows) {
                assert(row.size() == 40);
                assert(row[x] == '_' || row[x] == 'c' || row[x] == 'e');
                zappers += row[x] == 'e';
            }
            // A 2-tile-tall player always has room to pass
            assert(zappers <= 6);
        }
    }
}

int main()
{
    testPacketFraming();
    testIntegers();
    testMapChunks();
    testGenerator();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
