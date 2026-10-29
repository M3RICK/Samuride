// Self-checks run by make tests: packet framing, map streaming, map generator, power-ups

#include "map.hpp"
#include "protocol.hpp"
#include "messages.hpp"
#include "../server/generator.hpp"
#include "../server/player.hpp"
#include "../server/boss.hpp"
#include "../server/server.hpp"
#include "socket.hpp"
#include "../client/wallet.hpp"
#include "cyberpsycho.hpp"
#include "snipers.hpp"
#include "tuning.hpp"
#include "difficulty.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>
#include <map>
#include <optional>
#include <thread>

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

// Each message read back as it was written; a cut payload is refused
template <typename T, typename Same>
static void assertRoundTrip(const T &sent, Same same)
{
    std::vector<uint8_t> payload = Message::encode(sent);
    T read{};
    assert(Message::decode(payload, read) && same(sent, read));
    payload.pop_back();
    assert(!Message::decode(payload, read));
}

static void testMessages()
{
    PlayerState player{.x = 12.5f, .y = 3.25f, .score = 300, .jet_active = true, .form = Form::Titan, .shielded = true,
                       .ram = 2, .berserk = true, .ultimate = 80, .spears = 3, .grace = true, .style = 5};
    std::map<int, PlayerState> players = {{1, player}, {0, {}}};
    assertRoundTrip(players, [](auto &a, auto &b) {
        return a.size() == b.size() && std::ranges::all_of(a, [&](auto &entry) {
            auto [n, p] = entry;
            auto &q = b.at(n);
            return p.x == q.x && p.y == q.y && p.score == q.score && p.jet_active == q.jet_active && p.form == q.form &&
                   p.shielded == q.shielded && p.sprinting == q.sprinting && p.ram == q.ram && p.berserk == q.berserk &&
                   p.ultimate == q.ultimate && p.spears == q.spears && p.grace == q.grace && p.style == q.style;
        });
    });
    // The bytes themselves, as docs/PROTOCOL.md has them: other builds read these
    assert(Message::encode(std::map<int, PlayerState>{{1, player}}) ==
           (std::vector<uint8_t>{1, 0, 0, 0x04, 0xE2, 0x01, 0x45, 0x01, 0x2C, 0xCD, 80, 3, 11}));

    GameStart start{.my_number = 1, .roster = {{0, 3}, {1, 5}}, .district = 2, .mode = GameMode::Cyberpsycho};
    assertRoundTrip(start, [](auto &a, auto &b) {
        return a.my_number == b.my_number && a.roster == b.roster && a.district == b.district && a.mode == b.mode;
    });
    assert(Message::encode(start) == (std::vector<uint8_t>{1, 2, 0, 3, 1, 5, 2, 1}));

    assertRoundTrip(ShotEvent{2, Weapon::Spear}, [](auto &a, auto &b) {
        return a.player == b.player && a.weapon == b.weapon;
    });
    assertRoundTrip(UltimateEvent{1, Ultimate::Nuke, 140.5f, 4.75f}, [](auto &a, auto &b) {
        return a.player == b.player && a.ultimate == b.ultimate && a.x == b.x && a.y == b.y;
    });
    assertRoundTrip(std::vector<TitanState>{{TitanType::Armored, 320.25f}, {TitanType::Female, 9}}, [](auto &a, auto &b) {
        return a.size() == b.size() && a[0].type == b[0].type && a[0].x == b[0].x && a[1].type == b[1].type;
    });
    assertRoundTrip(std::vector<SniperState>{{SniperPhase::Firing, 6.5f}}, [](auto &a, auto &b) {
        return a.size() == b.size() && a[0].phase == b[0].phase && a[0].y == b[0].y;
    });
    assertRoundTrip(BossState{true, 45, BossAttack::Slam, BossPhase::Recover, 512.5f, 530, 7.25f, 1.5f, true, 0b1000000101}, [](auto &a, auto &b) {
        return a.fighting == b.fighting && a.health == b.health && a.attack == b.attack && a.phase == b.phase &&
               a.x == b.x && a.rockets_x == b.rockets_x && a.y == b.y && a.time == b.time &&
               a.sandevistan == b.sandevistan && a.reflected == b.reflected;
    });
    assertRoundTrip(CollisionEffect{TITAN_KILL, 1, 1234, 9}, [](auto &a, auto &b) {
        return a.type == b.type && a.player == b.player && a.x == b.x && a.y == b.y;
    });

    // Connect: a short one from an older client still goes through, the missing parts left out
    ConnectRequest connect{.character = 4, .district = 1, .mode = 1, .upgrades = {}};
    connect.upgrades[0] = 2;
    ConnectRequest read;
    assert(Message::decode(Message::encode(connect), read) && read.character == 4 && read.district == 1 &&
           read.mode == 1 && read.upgrades == connect.upgrades);
    assert(Message::decode({3}, read) && read.character == 3 && read.district == NOT_PICKED && read.upgrades == UpgradeLevels{});
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
// Mikasa takes off quicker than Johnny, to the same top speed
static void testAgility()
{
    Player johnny(-1), mikasa(-2);
    johnny.setCharacter(0);
    mikasa.setCharacter(3);
    for (Player *player : {&johnny, &mikasa}) {
        player->reset(0, 8);
        player->setJetActive(true);
        player->update(0.1f, 10, 8);
    }
    assert(agilityOf(3) > 1 && mikasa.getY() < johnny.getY());
}

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

// Wraith: the Blink takes him ahead, a perfect one leaves him untouchable a while; Stillness stops the world
// for STILLNESS_TIME, and Berserk doesn't blink for him
static void testWraith()
{
    assert(weaponOf(6) == Weapon::Blink && ultimateOf(6) == Ultimate::Stillness);

    Player wraith(-1);
    wraith.setCharacter(6);
    wraith.reset(0, 0);
    RunnerWeapon &blink = wraith.weapon();
    assert(blink.charges() == BLINK_CHARGES && !blink.autoFires());
    assert(blinkDistance(0) == BLINK_DISTANCE && blinkDistance(BLINK_HOLD_TIME * 2) == BLINK_FAR_DISTANCE);
    wraith.blinkTo(14);
    assert(wraith.getX() == 14 && !wraith.isGracing());
    wraith.dodge(0.5f);
    assert(wraith.isInvincible());

    ServerConfig config;
    Server server(config);      // never started: Stillness doesn't use it
    assert(!wraith.ultimate().freezes());
    wraith.ultimate().fire(server, wraith, 0, 0);
    assert(wraith.ultimate().freezes());
    wraith.update(STILLNESS_TIME - 0.1f, 10, 8);
    assert(wraith.ultimate().freezes());
    wraith.update(0.2f, 10, 8);
    assert(!wraith.ultimate().freezes());
}

// Against Smasher: the rockets Wraith blinks through turn back (and stop hitting), and his cannon about to
// fire threatens whoever is on its line, until it's over
static void testWraithSmasher()
{
    std::map<int, Player> players;
    std::mt19937 rng(2);
    players.emplace(3, Player(3));
    players.at(3).reset(0, 8);
    Boss boss;
    boss.reset();
    auto until = [&](auto done) {
        for (int i = 0; i < 20000 && !done(boss.state()); i++)
            boss.update(BOSS_DISTANCE, 0.01f, rng, players, 10);
        assert(done(boss.state()));
    };

    until([](const BossState &s) { return s.attack == BossAttack::Rockets && s.phase == BossPhase::Strike; });
    BossState wall = boss.state();
    int row = 0;
    while (!rocketRow(row, wall.y) || !rocketRow(row + 1, wall.y))
        row++;
    assert(boss.reflect(wall.rockets_x + 10, wall.rockets_x + 15, row, row + 2, 10) == 0);    // not there yet
    assert(rocketsHit(0, wall.y, row, 10));
    // A Blink landing short of the wall still reaches it, and takes the rows beside him too
    assert(boss.reflect(wall.rockets_x - 6, wall.rockets_x - BOSS_REFLECT_SLACK + 0.5f, row, row + 2, 10) >= 2);
    assert(boss.reflect(wall.rockets_x - 1, wall.rockets_x + 1, row, row + 2, 10) == 0);     // once each
    assert(!rocketsHit(0, wall.y, row, 10, boss.state().reflected));
    assert(!rocketsHit(0, wall.y, row - 0.4f, 10, boss.state().reflected) && !rocketsHit(0, wall.y, row + 0.4f, 10, boss.state().reflected));

    Player elsewhere(4);
    elsewhere.reset(1, 0);
    until([](const BossState &s) {
        return s.attack == BossAttack::Cannon && s.phase != BossPhase::Strike &&
               Boss::phaseOf(BossAttack::Cannon, s.time + BLINK_PERFECT_WINDOW) == BossPhase::Strike;
    });
    float strike = boss.threatens(players.at(3), 10, BLINK_PERFECT_WINDOW);
    assert(strike > BOSS_CANNON_TIME && strike <= BLINK_PERFECT_WINDOW + BOSS_CANNON_TIME + 0.01f);
    assert(boss.threatens(elsewhere, 10, BLINK_PERFECT_WINDOW) == 0);
    assert(boss.threatens(players.at(3), 10, 0.001f) == 0);     // not that close yet
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

    // Trauma Team: once per run, with grace, half the run's eddies billed; back after a reset
    levels = {};
    levels[UPGRADE_TRAUMA] = 1;
    levels[UPGRADE_SERUM] = 2;
    Player rescued(-1);
    rescued.setUpgrades(levels);
    rescued.reset(0, 0);
    rescued.addScore(101);
    assert(rescued.callTraumaTeam() && rescued.getScore() == 51 && rescued.isInvincible());
    assert(!rescued.callTraumaTeam());
    // Down on the floor while it works, then back with its grace
    assert(rescued.awaitRescue(TRAUMA_RESCUE_TIME - 0.1f, 8) && rescued.getY() == 8 && rescued.isGracing());
    assert(rescued.awaitRescue(0.2f, 8) && !rescued.awaitRescue(0.1f, 8));
    rescued.reset(0, 0);
    assert(rescued.callTraumaTeam());
    assert(!player.callTraumaTeam());

    // Titan Serum: the titan lasts a second longer per level
    assert(titanTime(levels) == TITAN_TIME + 2 * SERUM_PER_LEVEL);
    rescued.startShift();
    rescued.update(SHIFT_TIME + 0.01f, 10, 8);
    rescued.update(TITAN_TIME + 1.5f, 10, 8);
    assert(rescued.getForm() == Form::Titan);

    // Drone Pinata: a drone that drops nothing drops a power-up; the others keep theirs
    for (int x = 0; x < 50; x++) {
        char drop = droneDrop(x, 3);
        assert(isPowerUp(droneDrop(x, 3, true)) && (drop == EMPTY_TILE || droneDrop(x, 3, true) == drop));
    }
}

// The results screen's next goal: the cheapest runner or upgrade level left
static void testNextGoal()
{
    Wallet wallet;
    wallet.unlocked = 1;
    Wallet::Goal goal = wallet.nextGoal();
    assert(goal.runner == 1 && goal.price == RUNNERS[1].price);

    wallet.unlocked = RUNNER_COUNT;
    goal = wallet.nextGoal();
    assert(goal.runner < 0 && goal.upgrade >= 0 && goal.price == UPGRADES[goal.upgrade].prices[0]);

    for (int i = 0; i < UPGRADE_COUNT; i++)
        wallet.upgrades[i] = UPGRADES[i].max_level;
    assert(wallet.nextGoal().price < 0);
    wallet.upgrades[UPGRADE_LUCKY] = 1;
    goal = wallet.nextGoal();
    assert(goal.upgrade == UPGRADE_LUCKY && goal.price == UPGRADES[UPGRADE_LUCKY].prices[1]);
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
    assert(!boss.sandevistan() && !boss.damage(static_cast<int>(BOSS_HEALTH * (1 - BOSS_SANDEVISTAN_HEALTH))) && boss.sandevistan());
    assert(boss.state().sandevistan && !boss.damage(1) && boss.damage(BOSS_HEALTH) && !boss.fighting());
    assert(!boss.holds(BOSS_DISTANCE + 1));     // downed: the rest of his arena is a normal run

    boss.reset();
    boss.update(BOSS_DISTANCE, 0.1f, rng, players, 10);
    assert(!boss.sandevistan());
    boss.update(BOSS_DISTANCE + BOSS_ARENA / 2, 0.1f, rng, players, 10);
    assert(boss.sandevistan());                 // halfway: his Sandevistan, even unhurt
    // It hurries the gap before his next attack, not that attack's warning
    float before = boss.state().time;
    boss.update(BOSS_DISTANCE + BOSS_ARENA / 2, 0.1f, rng, players, 10);
    assert(std::abs(boss.state().time - before - 0.1f * BOSS_SANDEVISTAN_RATE) < 1e-4f);
    boss.update(BOSS_DISTANCE + BOSS_ARENA / 2, BOSS_INTRO_TIME, rng, players, 10);
    boss.update(BOSS_DISTANCE + BOSS_ARENA / 2, 0.1f, rng, players, 10);
    assert(boss.state().phase == BossPhase::Warning && std::abs(boss.state().time - 0.1f) < 1e-4f);
    boss.update(BOSS_DISTANCE + BOSS_ARENA, 0.1f, rng, players, 10);
    assert(!boss.fighting());
    assert(!inBossArena(BOSS_DISTANCE - 1) && inBossArena(BOSS_DISTANCE) && !inBossArena(BOSS_DISTANCE + BOSS_ARENA));
}

// A bare client for the end-to-end test: it talks to a real server over loopback
struct TestClient {
    int fd;
    std::vector<uint8_t> stream;

    explicit TestClient(int port, int version = PROTOCOL_VERSION) : fd(Socket::connect("127.0.0.1", port, 1000))
    {
        assert(fd >= 0);
        if (version > 0)
            send(MSG_VERSION, {static_cast<uint8_t>(version)});
    }

    void send(MessageType type, const std::vector<uint8_t> &payload)
    {
        assert(Socket::send(fd, Protocol::createPacket(type, payload)) > 0);
    }

    // The next packet of that type, the others skipped; nothing after 5 s
    std::optional<std::vector<uint8_t>> await(MessageType type)
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        MessageHeader header;
        std::vector<uint8_t> payload;

        while (std::chrono::steady_clock::now() < deadline) {
            while (Protocol::extractPacket(stream, header, payload))
                if (header.type == type)
                    return payload;
            std::vector<Socket::PollEntry> entries = {{fd}};
            if (Socket::poll(entries, 50) && entries[0].readable && !Socket::receive(fd, stream))
                return std::nullopt;
        }
        return std::nullopt;
    }

    // This player's latest state, a few ticks from now
    PlayerState latest(int number)
    {
        std::map<int, PlayerState> players;
        for (int ticks = 0; ticks < 10; ticks++) {
            auto payload = await(MSG_GAME_STATE);
            assert(payload && Message::decode(*payload, players));
        }
        return players.at(number);
    }
};

// Two players over real sockets: lobby, start, countdown, the run, someone leaving
static void testServerRun()
{
    const int port = 47123;
    ServerConfig config;
    config.port = port;
    config.quiet = true;
    Server server(config);
    assert(server.initialize());
    std::thread loop(&Server::run, &server);

    // Builds on another protocol, or from before there was one, are told ours and let go
    for (int version : {0, PROTOCOL_VERSION + 1}) {
        TestClient stranger(port, version);
        stranger.send(MSG_CONNECT, Message::encode(ConnectRequest{.character = 1, .district = 3, .mode = 1}));
        auto refusal = stranger.await(MSG_VERSION);
        assert(refusal && refusal->size() == 1 && (*refusal)[0] == PROTOCOL_VERSION);
        assert(!stranger.await(MSG_GAME_START));
        Socket::close(stranger.fd);
    }

    // Alice, first in, hosts: her district holds even when Bob's connect comes before hers
    TestClient alice(port);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TestClient bob(port);
    bob.send(MSG_CONNECT, Message::encode(ConnectRequest{.character = 4}));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    alice.send(MSG_CONNECT, Message::encode(ConnectRequest{.character = 2, .district = 1, .mode = 0}));

    // Everyone gets the map, then the same roster with their own number
    GameStart a_start;
    GameStart b_start;
    assert(alice.await(MSG_MAP_DATA) && bob.await(MSG_MAP_DATA));
    auto a_payload = alice.await(MSG_GAME_START);
    auto b_payload = bob.await(MSG_GAME_START);
    assert(a_payload && Message::decode(*a_payload, a_start));
    assert(b_payload && Message::decode(*b_payload, b_start));
    assert(a_start.my_number != b_start.my_number && a_start.roster == b_start.roster);
    assert(a_start.roster.at(a_start.my_number) == 2 && a_start.roster.at(b_start.my_number) == 4);
    assert(a_start.district == 1 && a_start.mode == GameMode::Classic);

    // Countdown down to 0, then the runners move: Alice holds her jetpack, Bob stays on the floor
    for (;;) {
        auto count = alice.await(MSG_COUNTDOWN);
        assert(count && !count->empty());
        if ((*count)[0] == 0)
            break;
    }
    PlayerState a0 = alice.latest(a_start.my_number);
    alice.send(MSG_PLAYER_INPUT, {1});
    PlayerState a1 = alice.latest(a_start.my_number);
    PlayerState b1 = alice.latest(b_start.my_number);
    assert(a1.x > a0.x && a1.jet_active && a1.y < a0.y);
    assert(!b1.jet_active && b1.y == a0.y);

    // Bob leaves: Alice wins
    Socket::close(bob.fd);
    auto end = alice.await(MSG_GAME_END);
    assert(end && !end->empty() && (*end)[0] == a_start.my_number);

    Socket::close(alice.fd);
    server.stop();
    loop.join();
}

// The Trauma Team asking first, over real sockets: a solo run into a wall of zappers, answered no, yes,
// then not at all
static void testTraumaOffer()
{
    std::string map_path = (std::filesystem::temp_directory_path() / "samuride_trauma_test.txt").string();
    {
        std::ofstream map(map_path);
        for (int row = 0; row < 10; row++)     // the height of the map the server makes
            map << std::string(20, EMPTY_TILE) << ZAPPER_TILE << std::string(20, EMPTY_TILE) << '\n';
    }
    float offer_time = TRAUMA_OFFER_TIME;
    TRAUMA_OFFER_TIME = 0.5f;

    // Into the wall with the Trauma Team bought; the next collision that isn't an eddy
    auto runInto = [&](int port, std::optional<bool> answer) {
        ServerConfig config;
        config.port = port;
        config.quiet = true;
        config.solo = true;
        config.map_path = map_path;
        Server server(config);
        assert(server.initialize());
        std::thread loop(&Server::run, &server);
        TestClient runner(port);
        ConnectRequest connect{.character = 0, .district = 0, .mode = 0};
        connect.upgrades[UPGRADE_TRAUMA] = 1;
        runner.send(MSG_CONNECT, Message::encode(connect));
        auto collision = [&] {
            CollisionEffect effect{};
            do {
                auto payload = runner.await(MSG_COLLISION);
                assert(payload && Message::decode(*payload, effect));
            } while (effect.type == COIN_TILE || effect.type == JACKPOT);
            return effect.type;
        };

        for (;;) {
            auto count = runner.await(MSG_COUNTDOWN);
            assert(count && !count->empty());
            if ((*count)[0] == 0)
                break;
        }
        assert(collision() == TRAUMA_OFFER);
        // The run waits on the answer
        float x = runner.latest(0).x;
        assert(runner.latest(0).x == x);
        if (answer)
            runner.send(MSG_TRAUMA, {static_cast<uint8_t>(*answer)});
        char outcome = collision();
        bool over = runner.await(MSG_GAME_END).has_value();
        Socket::close(runner.fd);
        server.stop();
        loop.join();
        return std::pair{outcome, over};
    };

    assert(runInto(47124, false) == std::pair(ZAPPER_TILE, true));
    assert(runInto(47125, true).first == TRAUMA_RESCUE);
    assert(runInto(47126, std::nullopt).first == TRAUMA_RESCUE);   // too late: it comes
    TRAUMA_OFFER_TIME = offer_time;
    std::filesystem::remove(map_path);
}

// Wraith over real sockets: eddies 2 tiles ahead and a wall of drones behind them, he blinks through both at the start
static void testBlink()
{
    std::string map_path = (std::filesystem::temp_directory_path() / "samuride_blink_test.txt").string();
    {
        std::ofstream map(map_path);
        for (int row = 0; row < 10; row++)
            map << std::string(2, EMPTY_TILE) << COIN_TILE << DRONE_TILE << std::string(30, EMPTY_TILE) << '\n';
    }
    ServerConfig config;
    config.port = 47127;
    config.quiet = true;
    config.solo = true;
    config.map_path = map_path;
    Server server(config);
    assert(server.initialize());
    std::thread loop(&Server::run, &server);
    TestClient wraith(config.port);
    wraith.send(MSG_CONNECT, Message::encode(ConnectRequest{.character = 6, .district = 0, .mode = 1}));
    for (;;) {
        auto count = wraith.await(MSG_COUNTDOWN);
        assert(count && !count->empty());
        if ((*count)[0] == 0)
            break;
    }
    float before = wraith.latest(0).x;
    wraith.send(MSG_ATTACK, {1});
    wraith.send(MSG_ATTACK, {0});

    // He took the drones in his way down, picked up the eddies he blinked over, and is past the wall alive
    bool drone = false, coin = false;
    while (!drone || !coin) {
        CollisionEffect effect{};
        auto payload = wraith.await(MSG_COLLISION);
        assert(payload && Message::decode(*payload, effect));
        drone |= effect.type == DRONE_TILE && effect.x == 3;
        coin |= effect.type == COIN_TILE && effect.x == 2;
    }
    PlayerState after = wraith.latest(0);
    assert(after.x >= before + BLINK_DISTANCE && after.ram == BLINK_CHARGES - 1);

    Socket::close(wraith.fd);
    server.stop();
    loop.join();
    std::filesystem::remove(map_path);
}

int main()
{
    testPacketFraming();
    testIntegers();
    testMessages();
    testServerRun();
    testTraumaOffer();
    testBlink();
    testMapChunks();
    testGenerator();
    testSniperHit();
    testBoss();
    testPowerUps();
    testHack();
    testAgility();
    testUltimate();
    testDante();
    testWraith();
    testWraithSmasher();
    testTuning();
    testWallet();
    testUpgrades();
    testNextGoal();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
