// Boons, Hades style (CYBERPSYCHO): at BOON_DISTANCE the run stops BOON_TIME seconds and each runner picks
// one of three, free, for the rest of the run. The first card is a general boon, the second a general one
// or the runner's own (Signature, by weapon), the third a pact: more eddies or heat, at a price.
// Shared by the server (who rolls and applies them) and the client (who shows them).

#ifndef BOONS_HPP
    #define BOONS_HPP

#include <cstdint>
#include "weapons.hpp"

enum class Boon : uint8_t {
    None,
    Trapped, ColdBlood, Notorious, Jackpot, Overclock, Rage,   // general
    GlassCannon, PriceOnHead,                                  // pacts
    Signature,                                                 // the runner's own
};
constexpr int FIRST_PACT = static_cast<int>(Boon::GlassCannon);
constexpr int PACTS_END = static_cast<int>(Boon::Signature);

inline float BOON_DISTANCE = 250;
inline float BOON_TIME = 10;
inline float COLD_BLOOD_AIM = 0.4f;     // seconds more the snipers' sight holds still before firing
inline float JACKPOT_FEVER = 0.08f;     // added to the chance an eddy is a jackpot
inline float GLASS_CANNON_EDDIES = 1.5f;
inline int PRICE_ON_HEAD_START = 3;     // stars right away, and never fewer than PRICE_ON_HEAD_FLOOR
inline int PRICE_ON_HEAD_FLOOR = 2;

// MSG_COLLISION types: the boons offered to a player (x: the three ids, a byte each, first card lowest);
// the one they took (x: its id, 0 for none)
constexpr char BOON_OFFER = 'B';
constexpr char BOON_TAKEN = 'G';

struct BoonLook {
    const char *name;
    const char *what;
};

inline BoonLook boonLook(Boon boon, Weapon weapon)
{
    switch (boon) {
        case Boon::Trapped: return {"BOOBY-TRAPPED DRONES", "A drone you down shorts the zappers next to it"};
        case Boon::ColdBlood: return {"COLD BLOOD", "Sniper sights hold still longer before the shot"};
        case Boon::Notorious: return {"NOTORIOUS", "Your wanted stars cool twice as slow"};
        case Boon::Jackpot: return {"JACKPOT FEVER", "8% of the eddies are jackpots"};
        case Boon::Overclock: return {"OVERCLOCK", "Sandevistan lasts twice as long"};
        case Boon::Rage: return {"BLOODLUST", "Berserk lasts twice as long"};
        case Boon::GlassCannon: return {"PACT: GLASS CANNON", "Eddies x1.5, but no shield and no Trauma Team"};
        case Boon::PriceOnHead: return {"PACT: PRICE ON YOUR HEAD", "3 stars now, never fewer than 2"};
        case Boon::Signature:
            switch (weapon) {
                case Weapon::Minigun: return {"BIGGER BURSTS", "Pow-Pow fires 5 a burst"};
                case Weapon::Blade: return {"LONG EDGE", "Your blades reach 50% further"};
                case Weapon::Hack: return {"OVERCLOCKED DECK", "RAM comes back twice as fast"};
                case Weapon::Rebellion: return {"SHOWOFF", "Your style rank drops twice as slow"};
                case Weapon::Blink: return {"VOID WALKER", "Blinks come back twice as fast"};
                default: return {"DOUBLE TAP", "Every shot fires two bullets"};
            }
        default: return {"", ""};
    }
}

#endif
