// Packets written and read field by field, in the same order on both ends

#include "messages.hpp"
#include <algorithm>

namespace {

class Writer {
public:
    std::vector<uint8_t> data;

    void u8(uint8_t value) { data.push_back(value); }
    void u16(uint16_t value) { Protocol::pushU16(data, value); }
    void u32(uint32_t value) { Protocol::pushU32(data, value); }
    // Positions, in 1/100 tile; nothing on the map is left of 0
    void pos16(float value) { u16(std::max(0.0f, value) * POSITION_SCALE); }
    void pos32(float value) { u32(std::max(0.0f, value) * POSITION_SCALE); }
};

// Reading past the end gives 0 and marks the payload short
class Reader {
    const std::vector<uint8_t> &data;
    size_t pos = 0;
    bool short_payload = false;

    bool has(size_t size)
    {
        if (pos + size <= data.size())
            return true;
        short_payload = true;
        return false;
    }

public:
    explicit Reader(const std::vector<uint8_t> &payload) : data(payload) {}

    uint8_t u8() { return has(1) ? data[pos++] : 0; }
    uint16_t u16() { return has(2) ? (pos += 2, Protocol::readU16(data.data() + pos - 2)) : 0; }
    uint32_t u32() { return has(4) ? (pos += 4, Protocol::readU32(data.data() + pos - 4)) : 0; }
    float pos16() { return u16() / POSITION_SCALE; }
    float pos32() { return u32() / POSITION_SCALE; }

    bool ok() const { return !short_payload; }
    bool done() const { return pos >= data.size(); }
};

}

// [runner][district][mode][upgrade levels...]; older clients may send fewer upgrades
std::vector<uint8_t> Message::encode(const ConnectRequest &connect)
{
    Writer w;
    w.u8(connect.character);
    w.u8(connect.district);
    w.u8(connect.mode);
    for (int level : connect.upgrades)
        w.u8(level);
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, ConnectRequest &connect)
{
    Reader r(payload);
    connect = {};
    connect.character = r.u8();
    if (!r.done())
        connect.district = r.u8();
    if (!r.done())
        connect.mode = r.u8();
    for (int &level : connect.upgrades)
        if (!r.done())
            level = r.u8();
    return true;
}

// [my number][player count]([number][runner])...[district][mode]
std::vector<uint8_t> Message::encode(const GameStart &start)
{
    Writer w;
    w.u8(start.my_number);
    w.u8(start.roster.size());
    for (auto [number, character] : start.roster) {
        w.u8(number);
        w.u8(character);
    }
    w.u8(start.district);
    w.u8(static_cast<uint8_t>(start.mode));
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, GameStart &start)
{
    Reader r(payload);
    start = {};
    start.my_number = r.u8();
    for (int count = r.u8(); count > 0 && r.ok(); count--) {
        int number = r.u8();
        start.roster[number] = r.u8();
    }
    start.district = r.u8();
    start.mode = static_cast<GameMode>(r.u8());
    return r.ok();
}

// Per player: [number][x, 4 bytes][y, 2][score, 2][flags][ultimate %][spears][flags 2]
std::vector<uint8_t> Message::encode(const std::map<int, PlayerState> &players)
{
    Writer w;
    for (const auto &[number, p] : players) {
        w.u8(number);
        w.pos32(p.x);
        w.pos16(p.y);
        w.u16(p.score);
        // bit 0: jetpack, bits 1-2: form, bit 3: shield, bit 4: Sandevistan, bits 5-6: RAM, bit 7: Berserk
        w.u8(p.jet_active | static_cast<uint8_t>(p.form) << 1 | p.shielded << 3 | p.sprinting << 4 |
             p.ram << 5 | p.berserk << 7);
        w.u8(p.ultimate);
        w.u8(p.spears);
        // bit 0: a second of grace, bits 1-3: Dante's style rank, bits 4-6: wanted level
        w.u8(p.grace | p.style << 1 | p.wanted << 4);
    }
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, std::map<int, PlayerState> &players)
{
    Reader r(payload);
    players.clear();
    while (!r.done()) {
        int number = r.u8();
        PlayerState p;
        p.x = r.pos32();
        p.y = r.pos16();
        p.score = r.u16();
        uint8_t flags = r.u8();
        p.jet_active = flags & 1;
        p.form = static_cast<Form>((flags >> 1) & 3);
        p.shielded = flags & 8;
        p.sprinting = flags & 16;
        p.ram = (flags >> 5) & 3;
        p.berserk = flags & 128;
        p.ultimate = r.u8();
        p.spears = r.u8();
        uint8_t flags2 = r.u8();
        p.grace = flags2 & 1;
        p.style = (flags2 >> 1) & 7;
        p.wanted = (flags2 >> 4) & 7;
        if (!r.ok())
            return false;
        players[number] = p;
    }
    return true;
}

// [player][weapon]
std::vector<uint8_t> Message::encode(const ShotEvent &shot)
{
    Writer w;
    w.u8(shot.player);
    w.u8(static_cast<uint8_t>(shot.weapon));
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, ShotEvent &shot)
{
    Reader r(payload);
    shot.player = r.u8();
    shot.weapon = static_cast<Weapon>(r.u8());
    return r.ok();
}

// [player][ultimate][x, 4 bytes][y, 2 bytes]
std::vector<uint8_t> Message::encode(const UltimateEvent &ultimate)
{
    Writer w;
    w.u8(ultimate.player);
    w.u8(static_cast<uint8_t>(ultimate.ultimate));
    w.pos32(ultimate.x);
    w.pos16(ultimate.y);
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, UltimateEvent &ultimate)
{
    Reader r(payload);
    ultimate.player = r.u8();
    ultimate.ultimate = static_cast<Ultimate>(r.u8());
    ultimate.x = r.pos32();
    ultimate.y = r.pos16();
    return r.ok();
}

// [count]([type][x, 4 bytes])...
std::vector<uint8_t> Message::encode(const std::vector<TitanState> &titans)
{
    Writer w;
    w.u8(titans.size());
    for (const TitanState &titan : titans) {
        w.u8(static_cast<uint8_t>(titan.type));
        w.pos32(titan.x);
    }
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, std::vector<TitanState> &titans)
{
    Reader r(payload);
    titans.clear();
    for (int count = r.u8(); count > 0 && r.ok(); count--) {
        auto type = static_cast<TitanType>(r.u8());
        titans.push_back({type, r.pos32()});
    }
    return r.ok();
}

// [count]([phase][y, 2 bytes])...
std::vector<uint8_t> Message::encode(const std::vector<SniperState> &snipers)
{
    Writer w;
    w.u8(snipers.size());
    for (const SniperState &sniper : snipers) {
        w.u8(static_cast<uint8_t>(sniper.phase));
        w.pos16(sniper.y);
    }
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, std::vector<SniperState> &snipers)
{
    Reader r(payload);
    snipers.clear();
    for (int count = r.u8(); count > 0 && r.ok(); count--) {
        auto phase = static_cast<SniperPhase>(r.u8());
        snipers.push_back({phase, r.pos16()});
    }
    return r.ok();
}

// [fighting][health %][attack][phase][x, 4 bytes][rockets' x, 4 bytes][y, 2 bytes][seconds into the phase, 2 bytes]
// [sandevistan][reflected rows, 2 bytes]
std::vector<uint8_t> Message::encode(const BossState &boss)
{
    Writer w;
    w.u8(boss.fighting);
    w.u8(boss.health);
    w.u8(static_cast<uint8_t>(boss.attack));
    w.u8(static_cast<uint8_t>(boss.phase));
    w.pos32(boss.x);
    w.pos32(boss.rockets_x);
    w.pos16(boss.y);
    w.pos16(boss.time);
    w.u8(boss.sandevistan);
    w.u16(boss.reflected);
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, BossState &boss)
{
    Reader r(payload);
    boss.fighting = r.u8() != 0;
    boss.health = r.u8();
    boss.attack = static_cast<BossAttack>(r.u8());
    boss.phase = static_cast<BossPhase>(r.u8());
    boss.x = r.pos32();
    boss.rockets_x = r.pos32();
    boss.y = r.pos16();
    boss.time = r.pos16();
    boss.sandevistan = r.u8() != 0;
    boss.reflected = r.u16();
    return r.ok();
}

// [type][player][x, 4 bytes][y, 2 bytes], in whole tiles
std::vector<uint8_t> Message::encode(const CollisionEffect &collision)
{
    Writer w;
    w.u8(collision.type);
    w.u8(collision.player);
    w.u32(collision.x);
    w.u16(collision.y);
    return w.data;
}

bool Message::decode(const std::vector<uint8_t> &payload, CollisionEffect &collision)
{
    Reader r(payload);
    collision.type = r.u8();
    collision.player = r.u8();
    collision.x = r.u32();
    collision.y = r.u16();
    return r.ok();
}
