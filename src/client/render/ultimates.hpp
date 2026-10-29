// The CYBERPSYCHO ultimates on screen (see ultimates.cpp)

#ifndef ULTIMATES_RENDER_HPP
    #define ULTIMATES_RENDER_HPP

#include <SFML/System.hpp>
#include <algorithm>
#include <vector>
#include "../state.hpp"

class Canvas;
struct Sprites;
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
        bool launched = false;
        bool blown = false;
        sf::Clock age;      // charging first, then flying

        float flying() const { return age.getElapsedTime().asSeconds() - FISHBONES_CHARGE_TIME; }
        float front() const { return start + std::clamp(FISHBONES_SPEED * flying(), 0.0f, FISHBONES_RANGE); }
    };

    // David's ultimate on screen: the shell flying, then the blast
    struct Nuke {
        float start;        // where it was fired, in tiles
        bool landed = false;
        sf::Clock age;
    };

    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<FireWave> fire_waves;
    std::vector<Nuke> nukes;
    std::vector<Rocket> rockets;
    std::vector<sf::Clock> breaches;    // since each of Lucy's breaches
    std::vector<std::pair<int, sf::Clock>> stillnesses;    // whose, and since when

    void start(const UltimateEvent &ultimate);
    void drawFireWaves();
    void drawNukes();
    void drawBreaches();
    void drawStillnesses();
    void drawDoubles();
    void drawRockets();

public:
    Ultimates(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio, Particles &particles);

    // True while this player's Stillness lasts: his Blinks go all the way
    bool stills(int player) const
    {
        return std::ranges::any_of(stillnesses, [&](const auto &still) { return still.first == player; });
    }
    void clear();
    // Starts the ultimates the server reported
    void handle(GameState &state);
    void draw();
};

#endif
