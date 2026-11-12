// Boons, Hades style (CYBERPSYCHO): every BOON_DISTANCE the run stops BOON_TIME seconds and each runner picks
// one of three, free, for the rest of the run; they add up, none comes twice. The first card is a general
// boon, the second a general one or one of the runner's own, the third a pact: something good, at a price.
// Shared by the server (who rolls and applies them) and the client (who shows them).

#ifndef BOONS_HPP
    #define BOONS_HPP

#include <cstdint>
#include "runners.hpp"

// A new one: its line here (before GlassCannon if general, before Signature if a pact), its card in
// boonLook, and what it does on the server (Player::hasBoon)
enum class Boon : uint8_t {
    None,
    Trapped, ColdBlood, Notorious, Jackpot, Overclock, Rage, Plating, Adrenaline, Scrapper, SecondWind, Magnet,  // general
    GlassCannon, PriceOnHead, HotPursuit,                      // pacts
    RunnerPact,                                                // the runner's own pact
    Signature, Signature2,                                     // the runner's own boons
    End,
};
constexpr int FIRST_PACT = static_cast<int>(Boon::GlassCannon);
constexpr int PACTS_END = static_cast<int>(Boon::Signature);
constexpr int BOONS_END = static_cast<int>(Boon::End);

inline bool isPact(Boon boon) { return static_cast<int>(boon) >= FIRST_PACT && static_cast<int>(boon) < PACTS_END; }
inline bool isOwn(Boon boon) { return boon == Boon::RunnerPact || boon == Boon::Signature || boon == Boon::Signature2; }

inline float BOON_DISTANCE = 250;
inline float BOON_TIME = 10;
inline float COLD_BLOOD_AIM = 0.4f;     // seconds more the snipers' sight holds still before firing
inline float JACKPOT_FEVER = 0.08f;     // added to the chance an eddy is a jackpot
inline float GLASS_CANNON_EDDIES = 1.5f;
inline int PRICE_ON_HEAD_START = 3;     // stars right away, and never fewer than PRICE_ON_HEAD_FLOOR
inline int PRICE_ON_HEAD_FLOOR = 2;
inline float ADRENALINE_RATE = 1.5f;    // the ultimate gauge fills that much faster
inline int SCRAPPER_EDDIES = 2;         // more for every drone down
inline float MAGNET_BOON_REACH = 1.5f;  // tiles further the eddies fly in from
inline float HOT_PURSUIT_SNIPERS = 2;   // snipers that many times as often
// The runners' own
inline float ROCKERBOY_COOLDOWN = 1.3f; // Johnny's pact: his gun's cooldown times this
inline float SPARE_SHELL = 0.3f;        // David: share of the gauge left after a nuke
inline float CHROME_JUNKIE_TIME = 3;    // David's pact: Sandevistan that many times as long
inline float TRIGGER_HAPPY = 0.7f;      // Jinx: the wait between two bursts times this
inline float GAS_OVERLOAD = 1.3f;       // Mikasa's pact: her agility times this
inline float OVERHEAT_RAM = 3;          // Lucy's pact: RAM back that many times as fast
inline int DEVIL_MAY_CRY_RANK = 2;      // Dante: his style never under this rank (B)
inline float PHASE_SHIFT_TIME = 0.3f;   // Wraith: seconds untouchable after a Blink
inline float BIG_BOOTS_REACH = 1.5f;    // DirtyBastrd: his kick's reach times this
inline int BAD_REPUTATION_EDDIES = 3;   // DirtyBastrd's pact: more for every drone down

// MSG_COLLISION types: the boons offered to a player (x: the three ids, a byte each, first card lowest);
// the one they took (x: its id, 0 for none)
constexpr char BOON_OFFER = 'B';
constexpr char BOON_TAKEN = 'G';

struct BoonLook {
    const char *name;
    const char *what;
};

// The card of a boon, for the runner `character` (runners.hpp): their own depend on who they are
inline BoonLook boonLook(Boon boon, int character)
{
    switch (boon) {
        case Boon::Trapped: return {"BOOBY-TRAPPED DRONES", "A drone you down shorts the zappers next to it"};
        case Boon::ColdBlood: return {"COLD BLOOD", "Sniper sights hold still longer before the shot"};
        case Boon::Notorious: return {"NOTORIOUS", "Your wanted stars cool twice as slow"};
        case Boon::Jackpot: return {"JACKPOT FEVER", "8% of the eddies are jackpots"};
        case Boon::Overclock: return {"OVERCLOCK", "Sandevistan lasts twice as long"};
        case Boon::Rage: return {"BLOODLUST", "Berserk lasts twice as long"};
        case Boon::Plating: return {"DOUBLE PLATING", "A shield now, and you can carry two at once"};
        case Boon::Adrenaline: return {"ADRENALINE", "Your ultimate charges 50% faster"};
        case Boon::Scrapper: return {"SCRAPPER", "Every drone down pays 2 more eddies"};
        case Boon::SecondWind: return {"SECOND WIND", "Untouchable twice as long after a hit or a power-up"};
        case Boon::Magnet: return {"MAGNETIC FIELD", "Eddies fly to you from further away"};
        case Boon::GlassCannon: return {"PACT: GLASS CANNON", "Eddies x1.5, but no shield and no Trauma Team"};
        case Boon::PriceOnHead: return {"PACT: PRICE ON YOUR HEAD", "3 stars now, never fewer than 2"};
        case Boon::HotPursuit: return {"PACT: HOT PURSUIT", "Drones pay double, snipers come twice as often"};
        case Boon::RunnerPact:
            switch (character) {
                case JOHNNY: return {"PACT: ROCKERBOY", "Ultimate charges twice as fast, your gun fires slower"};
                case DAVID: return {"PACT: CHROME JUNKIE", "Sandevistan lasts x3, but it breaks your shields as it ends"};
                case JINX: return {"PACT: LOOSE CANNON", "Pow-Pow always sprays, your ultimate charges half as fast"};
                case MIKASA: return {"PACT: GAS OVERLOAD", "You climb and fall 30% faster. No way back"};
                case LUCY: return {"PACT: OVERHEAT", "RAM comes back x3, but every hack heats you like a kill"};
                case DANTE: return {"PACT: ALL OR NOTHING", "A rank a kill, but your style drops twice as fast"};
                case WRAITH: return {"PACT: VOID HUNGER", "A Blink that kills is free, one that kills nothing costs two"};
                default: return {"PACT: BAD REPUTATION", "Drones pay 3 more eddies and heat you twice as much"};
            }
        case Boon::Signature:
            switch (character) {
                case JINX: return {"BIGGER BURSTS", "Pow-Pow fires 5 a burst"};
                case MIKASA: return {"LONG EDGE", "Your blades reach 50% further"};
                case LUCY: return {"OVERCLOCKED DECK", "RAM comes back twice as fast"};
                case DANTE: return {"SHOWOFF", "Your style rank drops twice as slow"};
                case WRAITH: return {"VOID WALKER", "Blinks come back twice as fast"};
                case DIRTYBASTRD: return {"STEEL TOES", "Drones you kick fly into the next one"};
                default: return {"DOUBLE TAP", "Every shot fires two bullets"};
            }
        case Boon::Signature2:
            switch (character) {
                case JOHNNY: return {"HOLLOW POINTS", "Your bullets hit drones twice as far off their line"};
                case DAVID: return {"SPARE SHELL", "Your gauge starts back at 30% after a nuke"};
                case JINX: return {"TRIGGER HAPPY", "Bursts come back 30% sooner"};
                case MIKASA: return {"NAPE HUNTER", "Titans pay double"};
                case LUCY: return {"DAISY CHAIN", "A hack jumps on to two drones instead of one"};
                case DANTE: return {"DEVIL MAY CRY", "Your style never drops below B"};
                case WRAITH: return {"PHASE SHIFT", "Untouchable for a moment after every Blink"};
                default: return {"BIG BOOTS", "Your kick reaches 50% further"};
            }
        default: return {"", ""};
    }
}

#endif
