// How the run moves and how hard it gets: speed ramp, jetpack physics, what the generator places.
// Run by the server; every balance value is listed in tuning.cpp, and assets/tuning.txt can change it.

#ifndef DIFFICULTY_HPP
    #define DIFFICULTY_HPP

// Run speed in tiles per second, ramps up over time: top speed after ~2 min, ~1500 m
inline float START_SPEED = 9.0f;
inline float MAX_SPEED = 16.0f;
inline float ACCELERATION = 0.06f;     // tiles per second, every second

// Map chunks are 40 tiles: obstacles are at their hardest after this many, ~1200 m
inline int CHUNKS_TO_MAX_DIFFICULTY = 30;

// The jetpack, in tiles per second (per second): gravity pulls down, the jetpack up
inline float GRAVITY = 28.0f;
inline float JET_POWER = 32.0f;
inline float MAX_VELOCITY = 10.0f;

// What the generator places next, rolled for each obstacle: drones first (CYBERPSYCHO), then a zapper,
// otherwise eddies. Zappers grow more frequent up to full difficulty.
inline float DRONE_CHANCE = 0.3f;
inline float ZAPPER_CHANCE = 0.15f;         // on top of the drones' share, at the start
inline float ZAPPER_CHANCE_GROWTH = 0.3f;   // added by full difficulty
// Empty columns after a zapper: at top speed 6 is about the time needed to switch lanes
inline int ZAPPER_GAP_MIN = 6;
inline int ZAPPER_GAP_MAX = 9;

#endif
