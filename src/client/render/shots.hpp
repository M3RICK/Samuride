// CYBERPSYCHO attacks: muzzle flashes, slashes, the projectiles in flight, and the ultimates.
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

    Canvas &canvas;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<Trail> trails;

    sf::Vector2f positionOf(const Trail &trail) const;
    void startUltimate(const UltimateEvent &ultimate);
    void drawFireWaves();

public:
    Shots(Canvas &canvas, Run &run, Audio &audio, Particles &particles);

    void clear() { trails.clear(); }
    // Starts the attacks the server reported
    void handle(GameState &state);
    // The server says one of `player`'s shots took a drone down: their oldest one in flight ends
    void land(int player);
    void draw();
};

#endif
