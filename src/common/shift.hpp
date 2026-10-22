// The Attack Titan power-up: grab the orb, stop while the smoke rises, then run as Eren's
// titan for a few seconds, invincible, before bursting back out of the smoke.

#ifndef SHIFT_HPP
    #define SHIFT_HPP

#include <cstdint>

enum class Form : uint8_t { Normal, Shifting, Titan, Emerging };

// Map tile of the power-up orb
constexpr char SHIFT_TILE = 'p';

inline float SHIFT_TIME = 1.8f;      // standing still in the smoke
inline float TITAN_TIME = 4.0f;      // running as the titan
inline float EMERGE_TIME = 1.0f;     // back to normal, still invincible

// The titan stands on the floor, half the screen tall
constexpr float TITAN_FORM_HEIGHT = 5.0f;
constexpr float TITAN_FORM_WIDTH = 2.5f;

// Map chunks are 40 tiles: no orb in the first ~300 m, then at least ~400 m between two,
// after which each chunk has this chance to hold one (about one orb every 500 m)
inline int SHIFT_FIRST_CHUNK = 8;
inline int SHIFT_MIN_GAP_CHUNKS = 10;
inline float SHIFT_CHANCE = 0.35f;

#endif
