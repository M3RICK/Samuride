// CYBERPSYCHO attacks: muzzle flashes, slashes and the projectiles in flight.
// Projectiles are drawn from the shooter's position and their age: the server moves them
// at run speed + weapon speed, the shooter at run speed, so the gap only depends on the weapon.

#ifndef SHOTS_HPP
    #define SHOTS_HPP

#include <SFML/System.hpp>
#include <set>
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
        sf::Clock age;
    };

    Canvas &canvas;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<Trail> trails;

    sf::Vector2f positionOf(const Trail &trail) const;

public:
    Shots(Canvas &canvas, Run &run, Audio &audio, Particles &particles);

    void clear() { trails.clear(); }
    // Starts the attacks the server reported
    void handle(GameState &state);
    void draw(const std::set<std::pair<int, int>> &destroyed);
};

#endif
