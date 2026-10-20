// The CYBERPSYCHO ultimates on screen (see ultimates.cpp)

#ifndef ULTIMATES_RENDER_HPP
    #define ULTIMATES_RENDER_HPP

#include <SFML/System.hpp>
#include <algorithm>
#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
class Particles;
struct Run;

class Ultimates {
private:
    // Johnny's ultimate on screen, moving like the server's
    struct FireWave {
        float start;        // in tiles
        float burnt_to;     // the zappers are out up to here
        sf::Clock age;

        float front() const { return start + std::min(FIRE_WAVE_RANGE, FIRE_WAVE_SPEED * age.getElapsedTime().asSeconds()); }
    };

    // Jinx's Fishbones on screen, moving like the server's
    struct Rocket {
        float start;        // in tiles
        float y;            // its line
        float burnt_to;     // the zappers of its band are out up to here
        bool blown = false;
        sf::Clock age;

        float front() const { return start + std::min(FISHBONES_RANGE, FISHBONES_SPEED * age.getElapsedTime().asSeconds()); }
    };

    // David's ultimate on screen: the shell flying, then the blast
    struct Nuke {
        float start;        // where it was fired, in tiles
        bool landed = false;
        sf::Clock age;
    };

    Canvas &canvas;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<FireWave> fire_waves;
    std::vector<Nuke> nukes;
    std::vector<Rocket> rockets;
    std::vector<sf::Clock> breaches;    // since each of Lucy's breaches

    void start(const UltimateEvent &ultimate);
    void drawFireWaves();
    void drawNukes();
    void drawBreaches();
    void drawRockets();

public:
    Ultimates(Canvas &canvas, Run &run, Audio &audio, Particles &particles);

    void clear();
    // Starts the ultimates the server reported
    void handle(GameState &state);
    void draw();
};

#endif
