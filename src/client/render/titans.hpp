// Titans running along the floor toward the player; a warning shows where one is about to burst in

#ifndef TITANS_RENDER_HPP
    #define TITANS_RENDER_HPP

#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
struct Sprites;
struct Run;

class Titans {
private:
    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    size_t on_screen = 0;   // how many were on screen last frame: a new one stomps in

    void drawTitan(const TitanState &titan, float time);

public:
    Titans(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio);

    void reset() { on_screen = 0; }
    void draw(const std::vector<TitanState> &titans);
    void drawWarnings(const std::vector<TitanState> &titans);
};

#endif
