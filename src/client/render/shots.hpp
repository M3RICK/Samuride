// CYBERPSYCHO attacks: muzzle flashes, slashes and the projectiles in flight (the ultimates: ultimates.hpp).
// Projectiles keep the height they were fired at, and are drawn ahead of the shooter by their age:
// the server moves them at run speed + weapon speed, the shooter at run speed, so the gap only
// depends on the weapon.

#ifndef SHOTS_HPP
    #define SHOTS_HPP

#include <SFML/System.hpp>
#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
class Particles;
struct Run;

class Shots {
private:
    struct Trail {
        int player;
        Weapon weapon;
        float y;        // in tiles, like on the server
        sf::Clock age;
    };

    // Wraith's Blink: from where he was to where the server puts him next, drawn once he's there
    struct Streak {
        int player;
        sf::Vector2f from;  // his body's middle, in tiles
        bool landed = false;
        sf::Clock age;
    };

    Canvas &canvas;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<Trail> trails;
    std::vector<Streak> streaks;

    sf::Vector2f positionOf(const Trail &trail) const;
    void drawStreaks();

public:
    Shots(Canvas &canvas, Run &run, Audio &audio, Particles &particles);

    void clear() { trails.clear(); streaks.clear(); }
    // Starts the attacks the server reported
    void handle(GameState &state);
    // The server says one of `player`'s shots hit at x: their one in flight there ends
    void land(int player, float x);
    void draw();
};

#endif
