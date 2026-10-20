// Self-checks run by make tests: packet framing, map streaming, map generator, power-ups

#include "map.hpp"
#include "protocol.hpp"
#include "../server/generator.hpp"
#include "../server/player.hpp"
#include "../client/wallet.hpp"
#include "cyberpsycho.hpp"
#include <cassert>
#include <iostream>
#include <map>

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

    for (int run = 0; run < 400; run++) {
        float difficulty = (run % 200) / 199.0f;
        bool drones = run >= 200;
        auto rows = generateChunk(rng, 40, 10, difficulty, drones);
        assert(rows.size() == 10);

        for (size_t x = 0; x < 40; x++) {
            int obstacles = 0;
            for (const auto &row : rows) {
                assert(row.size() == 40);
                assert(row[x] == '_' || row[x] == 'c' || row[x] == 'e' || (drones && row[x] == DRONE_TILE));
                obstacles += row[x] == 'e' || row[x] == DRONE_TILE;
            }
            // A 2-tile-tall player always has room to pass
            assert(obstacles <= 6);
        }
    }
}

// The shield takes one hit then gives a moment to get clear; the Sandevistan outruns everyone
static void testPowerUps()
{
    Player shielded(-1);
    shielded.reset(0, 0);
    assert(!shielded.absorbHit());
    shielded.powerUp(SHIELD_TILE);
    assert(shielded.hasShield() && shielded.absorbHit() && !shielded.hasShield());
    assert(shielded.isInvincible() && !shielded.absorbHit());
    for (int i = 0; i < 61; i++)
        shielded.update(1 / 60.f, 10, 8);
    assert(!shielded.isInvincible());

    Player sprinter(-1);
    sprinter.reset(1, 0);
    sprinter.powerUp(SANDEVISTAN_TILE);
    assert(sprinter.isSprinting() && sprinter.isInvincible());
    sprinter.update(1, 10, 8);
    assert(sprinter.getX() > 10 * SANDEVISTAN_SPEED - 0.01f);
    sprinter.update(SANDEVISTAN_TIME, 10, 8);
    assert(!sprinter.isSprinting());

    // Berserk: attacks come back twice as fast, for a while
    Player berserker(-1);
    berserker.reset(2, 0);
    berserker.powerUp(BERSERK_TILE);
    berserker.startCooldown(1);
    berserker.update(0.5f, 10, 8);
    assert(berserker.isBerserk() && berserker.canAttack());
    berserker.update(BERSERK_TIME, 10, 8);
    berserker.startCooldown(1);
    berserker.update(0.5f, 10, 8);
    assert(!berserker.isBerserk() && !berserker.canAttack());

    // About one drone in three drops something, all three kinds show up
    std::map<char, int> drops;
    int dropped = 0;
    for (int x = 0; x < 3000; x++) {
        char drop = droneDrop(x, x % 10);
        if (drop != '_') {
            drops[drop]++;
            dropped++;
        }
    }
    assert(dropped > 800 && dropped < 1200);
    assert(drops[SHIELD_TILE] > 200 && drops[SANDEVISTAN_TILE] > 200 && drops[BERSERK_TILE] > 200);
}

// Lucy's hack turns off a whole zapper and nothing else, and her RAM comes back one charge at a time
static void testHack()
{
    Map map;
    map.appendChunk({"e___", "_e_e", "__e_", "____"});
    std::vector<std::pair<int, int>> nodes = zapperNodes(map, 0, 0);
    assert(nodes.size() == 4);
    assert(zapperNodes(map, 3, 1).size() == 4);

    // A drone far above her is out of her way, the one level with her is hit even behind a closer column's
    Map lane;
    lane.appendChunk({"__d___", "______", "______", "______", "____d_", "______", "______"});
    auto none = [](int, int) { return false; };
    assert(hackTarget(lane, 0, 4, 2, none) == std::make_pair(4, 4));
    assert(hackTarget(lane, 0, 4, 2, [](int x, int) { return x == 4; }) == std::make_pair(-1, -1));

    Player lucy(-1);
    lucy.reset(0, 0);
    lucy.spendRam();
    lucy.spendRam();
    assert(lucy.getRam() == 0);
    lucy.update(RAM_RECHARGE - 0.1f, 10, 8);
    assert(lucy.getRam() == 0);
    lucy.update(0.2f, 10, 8);
    assert(lucy.getRam() == 1);
    lucy.update(RAM_RECHARGE + 0.1f, 10, 8);
    lucy.update(RAM_RECHARGE + 0.1f, 10, 8);
    assert(lucy.getRam() == RAM_MAX);
    assert(weaponOf(4) == Weapon::Hack);
}

// Every eddy charges the ultimate, up to full, and firing it empties the gauge
static void testUltimate()
{
    Player johnny(-1);
    johnny.reset(0, 0);
    for (int i = 0; i < 3; i++)
        johnny.collectCoin(i, 0);
    johnny.addScore(DRONE_BOUNTY);
    assert(johnny.getUltimateCharge() == 3 + DRONE_BOUNTY && !johnny.ultimateReady());
    johnny.addScore(ULTIMATE_CHARGE);
    assert(johnny.ultimateReady() && johnny.getUltimateCharge() == ULTIMATE_CHARGE);
    johnny.spendUltimate();
    assert(!johnny.ultimateReady() && johnny.getScore() == 3 + DRONE_BOUNTY + ULTIMATE_CHARGE);
    assert(ultimateOf(0) == Ultimate::FireWave && ultimateOf(1) == Ultimate::Nuke && ultimateOf(4) == Ultimate::Breach);
}

// Runners are bought in order, with enough eddies
static void testWallet()
{
    Wallet wallet;
    wallet.eddies = 120;
    assert(!wallet.buy(2) && !wallet.buy(0));
    assert(wallet.buy(1) && wallet.eddies == 20 && wallet.isUnlocked(1));
    assert(!wallet.buy(2) && !wallet.isUnlocked(2));
    wallet.eddies = 1000;
    assert(wallet.buy(2) && wallet.buy(3) && !wallet.buy(4));

    // Districts: free ones are there, the others in any order
    Wallet map_wallet;
    map_wallet.eddies = 400;
    assert(map_wallet.hasDistrict(0) && map_wallet.hasDistrict(2) && !map_wallet.hasDistrict(4));
    assert(map_wallet.buyDistrict(4) && map_wallet.hasDistrict(4) && map_wallet.eddies == 100);
    assert(!map_wallet.buyDistrict(4) && !map_wallet.buyDistrict(3) && !map_wallet.buyDistrict(0));
}

int main()
{
    testPacketFraming();
    testIntegers();
    testMapChunks();
    testGenerator();
    testPowerUps();
    testHack();
    testUltimate();
    testWallet();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
