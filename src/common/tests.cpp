// Self-checks run by make tests: packet framing, map streaming, map generator, power-ups

#include "map.hpp"
#include "protocol.hpp"
#include "../server/generator.hpp"
#include "../server/player.hpp"
#include "../server/boss.hpp"
#include "../client/wallet.hpp"
#include "cyberpsycho.hpp"
#include "snipers.hpp"
#include "tuning.hpp"
#include "difficulty.hpp"
#include <cmath>
#include <fstream>
#include <sstream>
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

// The railgun's beam hits the body from head to feet, and only along its row
static void testSniperHit()
{
    assert(sniperHits(6.0f, 5));
    assert(sniperHits(5.0f, 5));
    assert(sniperHits(7.0f, 5));
    assert(!sniperHits(4.3f, 5));
    assert(!sniperHits(7.7f, 5));
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

    assert(weaponOf(4) == Weapon::Hack);
    Player lucy(-1);
    lucy.setCharacter(4);
    lucy.reset(0, 0);
    auto &hack = dynamic_cast<Hack &>(lucy.weapon());
    for (int i = 0; i < RAM_MAX; i++)
        hack.spendRam();
    assert(hack.charges() == 0);
    lucy.update(RAM_RECHARGE - 0.1f, 10, 8);
    assert(hack.charges() == 0);
    lucy.update(0.2f, 10, 8);
    assert(hack.charges() == 1);
    for (int i = 0; i < RAM_MAX; i++)
        lucy.update(RAM_RECHARGE + 0.1f, 10, 8);
    assert(hack.charges() == RAM_MAX);
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
    johnny.addScore(ultimateCharge(0));
    assert(johnny.ultimateReady() && johnny.getUltimateCharge() == ultimateCharge(0));
    johnny.spendUltimate();
    assert(!johnny.ultimateReady() && johnny.getScore() == 3 + DRONE_BOUNTY + ultimateCharge(0));
    // Mikasa's fills sooner than the others'
    assert(ultimateCharge(3) < ultimateCharge(0));
    assert(ultimateOf(0) == Ultimate::FireWave && ultimateOf(1) == Ultimate::Nuke && ultimateOf(4) == Ultimate::Breach);
    assert(ultimateOf(3) == Ultimate::ThunderSpears && ultimateOf(2) == Ultimate::Fishbones);
    // An unknown runner plays as the first one
    assert(ultimateOf(RUNNER_COUNT) == Ultimate::FireWave && weaponOf(-1) == Weapon::Gun);

    Player mikasa(-1);
    mikasa.setCharacter(3);
    mikasa.reset(0, 0);
    auto &spears = dynamic_cast<ThunderSpears &>(mikasa.ultimate());
    spears.give();
    // One spear per press, their own cooldown between two, none left after the last
    assert(spears.throwSpear() && !spears.throwSpear());
    for (int i = 1; i < THUNDER_SPEARS; i++) {
        mikasa.update(1, 0, 8);
        assert(spears.throwSpear());
    }
    mikasa.update(1, 0, 8);
    assert(spears.charges() == 0 && !spears.throwSpear());

    // Pow-Pow: the two bullets after the first come MINIGUN_BURST_GAP apart, then nothing. The first is
    // fired by attack(), which needs a server: the burst is started by hand.
    Minigun powpow;
    powpow.startBurst();
    int fired = 0;
    for (int tick = 0; tick < 60; tick++)
        fired += powpow.burstShotDue(1 / 60.f);
    assert(fired == MINIGUN_BURST - 1);

    // Fishbones: Jinx stands still while it charges, its recoil throws her back, then she runs again
    Player thrown(-1);
    thrown.setCharacter(2);
    thrown.reset(0, 4);
    dynamic_cast<Fishbones &>(thrown.ultimate()).charge();
    thrown.update(FISHBONES_CHARGE_TIME / 2, 10, 8);
    assert(thrown.getX() == 0 && thrown.getY() == 4);
    thrown.update(FISHBONES_CHARGE_TIME / 2 + 0.01f, 10, 8);
    thrown.update(0.1f, 10, 8);
    assert(thrown.getX() < 0);
    thrown.update(FISHBONES_RECOIL_TIME, 10, 8);
    float before = thrown.getX();
    thrown.update(0.1f, 10, 8);
    assert(thrown.getX() > before + 0.99f);

    // Fishbones razes 5 rows around its line
    auto [top, bottom] = fishbonesRows(5);
    assert(top == 2 && bottom == 7);
}

// Dante: STYLE_KILLS kills go up a rank and the next drones pay more; a rank drops after
// STYLE_DROP_TIME without a kill, a hit drops STYLE_HIT_DROP ranks. His Doppelganger lasts DOPPELGANGER_TIME.
static void testDante()
{
    assert(weaponOf(5) == Weapon::Rebellion && ultimateOf(5) == Ultimate::Doppelganger);

    Player dante(-1);
    dante.setCharacter(5);
    dante.reset(0, 0);
    RunnerWeapon &rebellion = dante.weapon();
    for (int i = 0; i < STYLE_KILLS; i++)
        assert(rebellion.droneDown() == DRONE_BOUNTY);
    assert(rebellion.style() == 1);
    while (rebellion.style() < STYLE_RANKS - 1)
        rebellion.droneDown();
    assert(rebellion.droneDown() == static_cast<int>(std::lround(DRONE_BOUNTY * STYLE_BOUNTY[STYLE_RANKS - 1])));
    assert(rebellion.style() == STYLE_RANKS - 1);

    dante.update(STYLE_DROP_TIME - 0.1f, 10, 8);
    assert(rebellion.style() == STYLE_RANKS - 1);
    dante.update(0.2f, 10, 8);
    assert(rebellion.style() == STYLE_RANKS - 2);
    dante.powerUp(SHIELD_TILE);
    assert(dante.absorbHit() && rebellion.style() == STYLE_RANKS - 2 - STYLE_HIT_DROP);

    // Nobody else has a style rank
    Player johnny(-1);
    johnny.reset(0, 0);
    assert(johnny.weapon().droneDown() == DRONE_BOUNTY && johnny.weapon().style() == 0);

    assert(!dante.ultimate().doubles());
    dante.ultimate().tick(1);
    assert(!dante.ultimate().doubles());
    assert(mirrored(0, 2, 10) == 8 && mirrored(8, 2, 10) == 0);
}

// The tuning file: the shipped one is every default (regenerate it when one changes), a good line
// sets its value, a bad one changes nothing
static void testTuning()
{
    std::ifstream shipped("assets/tuning.txt");
    std::stringstream text;
    text << shipped.rdbuf();
    if (text.str() != Tuning::defaults())
        std::cerr << "assets/tuning.txt is out of date: ./Samuride_server -t > assets/tuning.txt" << std::endl;
    assert(text.str() == Tuning::defaults());
    assert(!Tuning::custom());

    std::istringstream lines("START_SPEED 9.5   # faster\nNO_SUCH 3\nDRONE_BOUNTY 2.5\nDRONE_CHANCE 4\nTITAN_BOUNTY\n");
    std::cerr.setstate(std::ios::failbit);
    assert(Tuning::apply(lines, "test") == 1);
    std::cerr.clear();
    assert(START_SPEED == 9.5f && DRONE_BOUNTY == 5 && DRONE_CHANCE == 0.3f && Tuning::custom());
    START_SPEED = 9;
    assert(!Tuning::custom());
}

// Upgrades: bought level by level up to their max, and felt by the player once sent to the server
static void testUpgrades()
{
    Wallet wallet;
    wallet.eddies = 3600;
    assert(wallet.buyUpgrade(UPGRADE_SANDEVISTAN) && wallet.buyUpgrade(UPGRADE_SANDEVISTAN));
    assert(wallet.eddies == 100 && wallet.upgrades[UPGRADE_SANDEVISTAN] == 2);
    assert(!wallet.buyUpgrade(UPGRADE_SANDEVISTAN) && !wallet.buyUpgrade(UPGRADE_COUNT));
    wallet.eddies = 100000;
    assert(wallet.buyUpgrade(UPGRADE_SHIELD) && !wallet.buyUpgrade(UPGRADE_SHIELD));

    UpgradeLevels levels{};
    levels[UPGRADE_SANDEVISTAN] = 2;
    levels[UPGRADE_SHIELD] = 1;
    Player player(-1);
    player.setUpgrades(levels);
    player.reset(0, 0);
    assert(player.hasShield());
    player.powerUp(SANDEVISTAN_TILE);
    player.update(SANDEVISTAN_TIME + 0.5f, 10, 8);
    assert(player.isSprinting());
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

// Adam Smasher: wakes up in his arena, the rocket wall leaves its gap free, the slam spares the air and
// leaves him close by for a while,
// he goes down at 0 health and leaves at the arena's end
static void testBoss()
{
    std::map<int, Player> players;
    std::mt19937 rng(1);
    players.emplace(3, Player(3));
    players.at(3).reset(0, 8);

    assert(!rocketsHit(0, 3, 3.5f, 10));        // inside the gap (rows 3 to 7)
    assert(rocketsHit(0, 3, 0, 10) && rocketsHit(0, 3, 8, 10));
    assert(!rocketsHit(5, 3, 0, 10));           // still far ahead
    assert(bandHits(10 - BOSS_SLAM_HEIGHT, 10, 8) && !bandHits(10 - BOSS_SLAM_HEIGHT, 10, 4));
    assert(Boss::phaseOf(BossAttack::Cannon, BOSS_CANNON_TRACK + 0.1f) == BossPhase::Aim);
    assert(Boss::phaseOf(BossAttack::Slam, BOSS_SLAM_WARN + BOSS_SLAM_TIME + 0.1f) == BossPhase::Recover);

    Boss boss;
    boss.reset();
    boss.update(BOSS_DISTANCE - 1, 0.1f, rng, players, 10);
    assert(!boss.fighting());
    boss.update(BOSS_DISTANCE, 0.1f, rng, players, 10);
    assert(boss.fighting() && boss.x() == BOSS_DISTANCE + BOSS_AHEAD);
    assert(!boss.hit(players.at(3), 10));       // his entrance: nothing yet
    assert(!boss.damage(BOSS_HEALTH - 1) && boss.damage(1) && !boss.fighting());

    boss.reset();
    boss.update(BOSS_DISTANCE, 0.1f, rng, players, 10);
    boss.update(BOSS_DISTANCE + BOSS_ARENA, 0.1f, rng, players, 10);
    assert(!boss.fighting());
    assert(!inBossArena(BOSS_DISTANCE - 1) && inBossArena(BOSS_DISTANCE) && !inBossArena(BOSS_DISTANCE + BOSS_ARENA));
}

int main()
{
    testPacketFraming();
    testIntegers();
    testMapChunks();
    testGenerator();
    testSniperHit();
    testBoss();
    testPowerUps();
    testHack();
    testUltimate();
    testDante();
    testTuning();
    testWallet();
    testUpgrades();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
