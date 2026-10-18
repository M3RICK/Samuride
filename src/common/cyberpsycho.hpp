#ifndef CYBERPSYCHO_HPP
    #define CYBERPSYCHO_HPP

#include <cstdint>

// Rules of the CYBERPSYCHO mode, shared by the server (who decides) and the client (who draws)

enum class GameMode : uint8_t { Classic, Cyberpsycho };

// Map tile of a security drone: deadly on contact, worth DRONE_BOUNTY eddies when destroyed
constexpr char DRONE_TILE = 'd';
constexpr int DRONE_BOUNTY = 5;

// Projectiles vanish this many tiles ahead of where they were fired
constexpr float PROJECTILE_RANGE = 22;

enum class Weapon : uint8_t { Gun, Rockets, Blade };

struct WeaponStats {
    float cooldown;     // seconds between two attacks
    float speed;        // projectile speed in tiles per second, on top of the run speed; 0 for melee
    float reach;        // how far ahead a melee strike lands, in tiles
};

// Character ids follow the client's CHARACTER_DIRS: johny, david, jinx, mikasa
inline Weapon weaponOf(int character)
{
    if (character == 2)
        return Weapon::Rockets;
    if (character == 3)
        return Weapon::Blade;
    return Weapon::Gun;
}

inline WeaponStats statsOf(Weapon weapon)
{
    switch (weapon) {
        case Weapon::Rockets:
            return {0.6f, 18, 0};
        case Weapon::Blade:
            return {0.45f, 0, 2.5f};
        default:
            return {0.25f, 30, 0};
    }
}

#endif
