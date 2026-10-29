// Every balance value assets/tuning.txt can change, with what it does. A new one: make its constant
// `inline` instead of `constexpr`, add its line here, then regenerate the file (./Samuride_server -t).

#include "tuning.hpp"
#include "difficulty.hpp"
#include "cyberpsycho.hpp"
#include "powerups.hpp"
#include "titans.hpp"
#include "snipers.hpp"
#include "boss.hpp"
#include "shift.hpp"
#include "upgrades.hpp"
#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

// A value, or a heading when it points to nothing
struct Entry {
    std::string name;
    std::string help;
    float *real = nullptr;
    int *whole = nullptr;
    float min = 0;
    float max = 1e6f;
    float fallback = 0;     // its default, kept to tell a custom file

    Entry(std::string heading) : help(std::move(heading)) {}
    Entry(std::string name, float *value, std::string help, float min = 0, float max = 1e6f)
        : name(std::move(name)), help(std::move(help)), real(value), min(min), max(max), fallback(*value) {}
    Entry(std::string name, int *value, std::string help, float min = 0, float max = 1e6f)
        : name(std::move(name)), help(std::move(help)), whole(value), min(min), max(max), fallback(*value) {}

    bool heading() const { return !real && !whole; }
    float get() const { return real ? *real : *whole; }
};

#define VALUE(constant, ...) Entry(#constant, &constant, __VA_ARGS__)

// Built on first use, before any file is read: the values it captures are the code's defaults
std::vector<Entry> &entries()
{
    static std::vector<Entry> list = [] {
        std::vector<Entry> all = {
            Entry("The run"),
            VALUE(START_SPEED, "Run speed when a run starts, in tiles per second"),
            VALUE(MAX_SPEED, "Top run speed"),
            VALUE(ACCELERATION, "Run speed gained every second, up to the top speed"),
            VALUE(CHUNKS_TO_MAX_DIFFICULTY, "Obstacles are at their hardest after this many 40-tile chunks (30 is ~1200 m)", 1),
            VALUE(GRAVITY, "How fast you fall, in tiles per second per second"),
            VALUE(JET_POWER, "How fast the jetpack lifts you"),
            VALUE(MAX_VELOCITY, "Fastest you can rise or fall, in tiles per second"),
            VALUE(DRONE_CHANCE, "Share of obstacles that are drones, in CYBERPSYCHO (0 to 1)", 0, 1),
            VALUE(ZAPPER_CHANCE, "Share of zappers at the start, after the drones' (0 to 1)", 0, 1),
            VALUE(ZAPPER_CHANCE_GROWTH, "Share of zappers added by full difficulty", 0, 1),
            VALUE(ZAPPER_GAP_MIN, "Fewest empty columns after a zapper"),
            VALUE(ZAPPER_GAP_MAX, "Most empty columns after a zapper"),

            Entry("Power-ups"),
            VALUE(POWERUP_FIRST_CHUNK, "No power-up in the first this many 40-tile chunks"),
            VALUE(POWERUP_CHANCE, "Chance a chunk holds a power-up after that (0 to 1)", 0, 1),
            VALUE(POWERUP_CHANCE_CYBERPSYCHO, "The same in CYBERPSYCHO, where drones drop them too", 0, 1),
            VALUE(SHIELD_GRACE, "Seconds of invincibility after the shield breaks"),
            VALUE(END_GRACE, "Seconds of invincibility after the Sandevistan or the Attack Titan ends"),
            VALUE(SANDEVISTAN_TIME, "Seconds of Sandevistan sprint"),
            VALUE(SANDEVISTAN_SPEED, "Sandevistan speed, times the run speed"),
            VALUE(BERSERK_TIME, "Seconds of Berserk"),
            VALUE(BERSERK_ATTACK_RATE, "Berserk attack cooldowns run out this many times faster"),
            VALUE(BERSERK_RAM_RATE, "Lucy's RAM comes back this many times faster in Berserk"),
            VALUE(SHIFT_FIRST_CHUNK, "No Attack Titan orb in the first this many chunks"),
            VALUE(SHIFT_MIN_GAP_CHUNKS, "At least this many chunks between two orbs"),
            VALUE(SHIFT_CHANCE, "Chance a chunk holds an orb after that (0 to 1)", 0, 1),
            VALUE(SHIFT_TIME, "Seconds standing still in the smoke before the titan comes out"),
            VALUE(TITAN_TIME, "Seconds running as the Attack Titan"),
            VALUE(EMERGE_TIME, "Seconds bursting out of the titan, still invincible"),
            VALUE(SANDEVISTAN_PER_LEVEL, "Seconds of Sandevistan each shop level adds"),
            VALUE(MAGNET_PER_LEVEL, "Tiles each eddie magnet level reaches"),
            VALUE(TRAUMA_OFFER_TIME, "Seconds you get to say whether to call the Trauma Team, after which it comes"),
            VALUE(TRAUMA_RESCUE_TIME, "Seconds the run waits while the Trauma Team picks you up"),
            VALUE(TRAUMA_GRACE, "Seconds of invincibility after the Trauma Team picks you up"),
            VALUE(TRAUMA_BILL, "Share of the run's eddies the Trauma Team bills (0 to 1)", 0, 1),
            VALUE(JACKPOT_PER_LEVEL, "Chance an eddy hits the jackpot, per Lucky Chip level (0 to 1)", 0, 1),
            VALUE(JACKPOT_EDDIES, "Eddies a jackpot is worth"),
            VALUE(SERUM_PER_LEVEL, "Seconds of Attack Titan each Titan Serum level adds"),
            VALUE(PINATA_PER_LEVEL, "Chance a drone that drops nothing drops a power-up, per Drone Pinata level (0 to 1)", 0, 1),

            Entry("Enemies"),
            VALUE(DRONE_BOUNTY, "Eddies for a drone down"),
            VALUE(TITAN_FIRST_DISTANCE, "No titan before this many tiles"),
            VALUE(TITAN_MIN_INTERVAL, "Fewest seconds between two titans"),
            VALUE(TITAN_MAX_INTERVAL, "Most seconds between two titans"),
            VALUE(TITAN_SPAWN_AHEAD, "Titans appear this many tiles ahead of the leader: their warning"),
            VALUE(TITAN_BOUNTY, "Eddies for a titan down"),
            VALUE(SNIPER_FIRST_DISTANCE, "No sniper before this many tiles"),
            VALUE(SNIPER_MIN_INTERVAL, "Fewest seconds between two snipers"),
            VALUE(SNIPER_MAX_INTERVAL, "Most seconds between two snipers"),
            VALUE(SNIPER_TRACK_TIME, "Seconds the laser sight follows you"),
            VALUE(SNIPER_AIM_TIME, "Seconds it stays frozen before the shot"),
            VALUE(SNIPER_FIRE_TIME, "Seconds the railgun beam lasts"),
            VALUE(SNIPER_BEAM_HALF, "Half the height of the beam, in tiles"),

            Entry("Adam Smasher, the boss"),
            VALUE(BOSS_DISTANCE, "Tiles before his arena"),
            VALUE(BOSS_ARENA, "How long his arena lasts, in tiles: survive it (CLASSIC) or shoot him down (CYBERPSYCHO)"),
            VALUE(BOSS_AHEAD, "Tiles ahead of the leader he stands"),
            VALUE(BOSS_INTRO_TIME, "Seconds of his entrance before his first attack"),
            VALUE(BOSS_ATTACK_GAP, "Seconds between two attacks"),
            VALUE(BOSS_SANDEVISTAN_HEALTH, "Share of his health under which his Sandevistan kicks in, as it does halfway through his arena (0 to 1)", 0, 1),
            VALUE(BOSS_SANDEVISTAN_RATE, "How many times faster his strikes and the gaps between them go under his Sandevistan (warnings stay as they are)"),
            VALUE(BOSS_HEALTH, "His health in CYBERPSYCHO", 1, 10000),
            VALUE(BOSS_SHOT_DAMAGE, "A bullet's damage"),
            VALUE(BOSS_SPEAR_DAMAGE, "A thunder spear's damage"),
            VALUE(BOSS_HACK_DAMAGE, "Lucy's hack's damage"),
            VALUE(BOSS_SLASH_DAMAGE, "A blade's damage (Mikasa, Dante's sword), while he is close after a slam"),
            VALUE(BOSS_STAB_DAMAGE, "Wraith blinking into him"),
            VALUE(BOSS_REFLECT_DAMAGE, "Each rocket Wraith blinks through, sent back at him"),
            VALUE(BOSS_REFLECT_SLACK, "Tiles past where Wraith's Blink lands that it still sends the rockets back"),
            VALUE(BOSS_REFLECT_GRACE, "Seconds Wraith can't be hurt after sending rockets back"),
            VALUE(BOSS_ULTIMATE_DAMAGE, "Any ultimate's damage"),
            VALUE(BOSS_BOUNTY, "Eddies for downing him"),
            VALUE(BOSS_ROCKET_WARN, "Seconds the rockets are warned"),
            VALUE(BOSS_ROCKET_SPEED, "Their speed toward you, tiles per second"),
            VALUE(BOSS_ROCKET_GAP, "Tiles of the gap in their wall", 2.5f, 10),
            VALUE(BOSS_SLAM_WARN, "Seconds the slam is warned"),
            VALUE(BOSS_SLAM_TIME, "Seconds its shockwave lasts"),
            VALUE(BOSS_SLAM_HEIGHT, "Tiles of floor it covers", 0, 7),
            VALUE(BOSS_SLAM_LAND, "Tiles ahead of you he lands"),
            VALUE(BOSS_SLAM_RECOVER, "Seconds he stays there, in reach of the blades"),
            VALUE(BOSS_CANNON_TRACK, "Seconds his cannon's sight follows you"),
            VALUE(BOSS_CANNON_AIM, "Seconds it stays frozen"),
            VALUE(BOSS_CANNON_TIME, "Seconds its beam lasts"),
            VALUE(BOSS_CANNON_HALF, "Half the height of the beam, in tiles"),

            Entry("Weapons (CYBERPSYCHO): cooldown in seconds, speed in tiles per second on top of the run, reach and radius in tiles"),
            VALUE(PROJECTILE_RANGE, "Shots vanish this many tiles ahead"),
            Entry("GUN_COOLDOWN", &WEAPON_STATS[0].cooldown, "Johnny and David"),
            Entry("GUN_SPEED", &WEAPON_STATS[0].speed, ""),
            Entry("GUN_RADIUS", &WEAPON_STATS[0].radius, "A shot hits drones this close above or below its line"),
            Entry("MINIGUN_COOLDOWN", &WEAPON_STATS[1].cooldown, "Jinx's Pow-Pow, between two bursts"),
            Entry("MINIGUN_SPEED", &WEAPON_STATS[1].speed, ""),
            Entry("MINIGUN_RADIUS", &WEAPON_STATS[1].radius, ""),
            VALUE(MINIGUN_BURST, "Bullets in a burst", 1, 20),
            VALUE(MINIGUN_BURST_GAP, "Seconds between the bullets of a burst"),
            VALUE(MINIGUN_SPRAY_COOLDOWN, "Seconds between two bullets in Berserk, before its speed-up"),
            Entry("BLADE_COOLDOWN", &WEAPON_STATS[2].cooldown, "Mikasa"),
            Entry("BLADE_REACH", &WEAPON_STATS[2].reach, ""),
            Entry("HACK_COOLDOWN", &WEAPON_STATS[3].cooldown, "Lucy"),
            VALUE(HACK_RANGE, "Tiles ahead she hacks"),
            VALUE(HACK_LANE, "Rows above her head and below her feet she hacks"),
            VALUE(HACK_TIME, "Seconds a hacked zapper stays off"),
            VALUE(HACK_BOUNCE_RANGE, "A hacked drone passes the hack to the nearest drone this close"),
            VALUE(RAM_RECHARGE, "Seconds to get one charge of RAM back (3 at most)"),
            VALUE(BLINK_DISTANCE, "Wraith: tiles ahead a Blink takes him"),
            VALUE(BLINK_FAR_DISTANCE, "Wraith: tiles ahead a Blink takes him, the key held long enough"),
            VALUE(BLINK_HOLD_TIME, "Seconds to hold the key for the farthest Blink"),
            VALUE(BLINK_CLOSE_CALL_BOUNTY, "Eddies for a Blink through a zapper or a drone"),
            VALUE(BLINK_COOLDOWN, "Seconds between two Blinks"),
            VALUE(BLINK_RECHARGE, "Seconds to get one Blink back (3 at most)"),
            VALUE(BLINK_PERFECT_WINDOW, "Blinking this many seconds before Smasher's slam or cannon hits him: through it, and the charge back"),

            Entry("Ultimates (CYBERPSYCHO)"),
            VALUE(FIRE_WAVE_SPEED, "Johnny: the fire wave's speed, tiles per second"),
            VALUE(FIRE_WAVE_RANGE, "How far it sweeps"),
            VALUE(NUKE_DELAY, "David: seconds before the nuke lands"),
            VALUE(NUKE_DISTANCE, "Tiles ahead it lands"),
            VALUE(NUKE_RADIUS, "Tiles razed on each side"),
            VALUE(BREACH_AHEAD, "Lucy: tiles ahead the breach reaches"),
            VALUE(BREACH_TIME, "Seconds the zappers stay off"),
            VALUE(THUNDER_SPEARS, "Mikasa: spears per ultimate", 0, 20),
            Entry("SPEAR_COOLDOWN", &WEAPON_STATS[4].cooldown, "Seconds between two spears"),
            Entry("SPEAR_SPEED", &WEAPON_STATS[4].speed, ""),
            Entry("SPEAR_RADIUS", &WEAPON_STATS[4].radius, ""),
            VALUE(SPEAR_BLAST_RADIUS, "Tiles on each side of a spear's lightning column"),
            VALUE(FISHBONES_SPEED, "Jinx: Fishbones' speed"),
            VALUE(FISHBONES_RANGE, "How far it flies"),
            VALUE(FISHBONES_HALF_HEIGHT, "Tiles razed above and below its line"),
            VALUE(FISHBONES_CHARGE_TIME, "Seconds she hangs in the air charging it"),
            VALUE(FISHBONES_RECOIL_TIME, "Seconds its recoil lasts"),
            VALUE(FISHBONES_RECOIL_PUSH, "How hard the recoil throws her back, times the run speed"),
            VALUE(DOPPELGANGER_TIME, "Dante: seconds his Doppelganger repeats his attacks"),
            VALUE(STILLNESS_TIME, "Wraith: seconds the world stands still"),

            Entry("Dante's style meter: ranks D, C, B, A, S, SS, SSS"),
            VALUE(STYLE_KILLS, "Kills in a row to go up a rank"),
            VALUE(STYLE_DROP_TIME, "Seconds without a kill to drop a rank"),
            VALUE(STYLE_HIT_DROP, "Ranks a hit (his shield breaking) drops", 0, 6),

        };
        for (int rank = 0; rank < STYLE_RANKS; rank++)
            all.emplace_back(std::string("STYLE_BOUNTY_") + STYLE_NAMES[rank], &STYLE_BOUNTY[rank],
                             rank == 0 ? "A drone's bounty times this, at each rank" : "");
        for (int rank = 0; rank < STYLE_RANKS; rank++)
            all.emplace_back(std::string("STYLE_HASTE_") + STYLE_NAMES[rank], &STYLE_HASTE[rank],
                             rank == 0 ? "His attack cooldowns times this, at each rank" : "");
        all.emplace_back("Runners: price in eddies, eddies to charge the ultimate, and agility: the jetpack's lift "
                         "and the fall times this, how fast they turn in the air (not their top speed)");
        for (Runner &runner : RUNNERS) {
            std::string name = runner.dir;
            std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::toupper(c); });
            all.emplace_back(name + "_PRICE", &runner.price, "");
            all.emplace_back(name + "_ULTIMATE_CHARGE", &runner.charge, "", 1);
            all.emplace_back(name + "_AGILITY", &runner.agility, "", 0.5f, 3);
        }
        return all;
    }();
    return list;
}

Entry *find(const std::string &name)
{
    auto it = std::ranges::find_if(entries(), [&](const Entry &entry) { return !entry.heading() && entry.name == name; });
    return it != entries().end() ? &*it : nullptr;
}

std::string show(float value)
{
    return std::format("{}", value);
}

} // namespace

int Tuning::load(const std::string &path)
{
    std::ifstream file(path);

    return file ? Tuning::apply(file, path) : 0;
}

int Tuning::apply(std::istream &in, const std::string &source)
{
    std::string line;
    int read = 0;

    for (int number = 1; std::getline(in, line); number++) {
        std::istringstream words(line.substr(0, line.find('#')));
        std::string name, value, extra;
        if (!(words >> name))
            continue;

        Entry *entry = find(name);
        std::string where = std::format("{}:{}: ", source, number);
        if (!entry) {
            std::cerr << where << "unknown value " << name << ", skipped" << std::endl;
            continue;
        }
        size_t used = 0;
        float number_read = 0;
        words >> value;
        try {
            number_read = std::stof(value, &used);
        } catch (const std::exception &) {
            used = 0;
        }
        bool whole = !entry->whole || number_read == static_cast<int>(number_read);
        if (value.empty() || used != value.size() || (words >> extra) || !whole) {
            std::cerr << where << name << " needs " << (entry->whole ? "a whole number" : "a number") << ", skipped" << std::endl;
            continue;
        }
        if (number_read < entry->min || number_read > entry->max) {
            std::cerr << where << name << " must be between " << show(entry->min) << " and " << show(entry->max)
                      << ", skipped" << std::endl;
            continue;
        }
        if (entry->real)
            *entry->real = number_read;
        else
            *entry->whole = static_cast<int>(number_read);
        read++;
    }
    return read;
}

std::string Tuning::defaults()
{
    std::string text =
        "# Samuride tuning: the game's balance, in one file.\n"
        "#\n"
        "# Change a number, save, restart the game. Delete a line to get its default back.\n"
        "# Distances are in tiles: a runner is 2 tiles tall, a tile is about a metre. Times are in seconds.\n"
        "# In versus, the host's file decides what happens; the other player only draws with theirs.\n"
        "# While a value isn't at its default, the game says CUSTOM TUNING in a corner.\n"
        "# Testing something? Send this file with what you felt: it says exactly what you played.\n"
        "#\n"
        "# Words used below:\n"
        "#   tile       the grid the map is drawn on: a square about a metre wide. The screen is 10 tiles\n"
        "#              tall and 20 wide; a runner is 2 tiles tall. The distance on screen counts tiles.\n"
        "#   row        a line of tiles across the screen, from top to bottom: 10 rows. Column: the same, upright.\n"
        "#   chunk      the map is made 40 tiles at a time, as you run, each piece drawn at random: a chunk.\n"
        "#              So 3 chunks is 120 m, 30 chunks is 1200 m.\n"
        "#   chance     from 0 (never) to 1 (always): 0.2 is one time in five.\n"
        "#   leader     the runner furthest ahead (in solo, you).\n"
        "#   cooldown   seconds before you can attack again.\n"
        "#   reach      how far ahead a melee attack lands. Radius: how close to a shot's line a drone gets hit.\n"
        "#   grace      a moment of invincibility, to get clear after a shield breaks or a power-up ends.\n";


    for (const Entry &entry : entries()) {
        if (entry.heading()) {
            text += "\n## " + entry.help + "\n";
            continue;
        }
        if (!entry.help.empty())
            text += "# " + entry.help + "\n";
        text += entry.name + " " + show(entry.fallback) + "\n";
    }
    return text;
}

bool Tuning::custom()
{
    return std::ranges::any_of(entries(), [](const Entry &entry) { return !entry.heading() && entry.get() != entry.fallback; });
}
