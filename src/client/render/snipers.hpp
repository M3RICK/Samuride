// Arasaka snipers: their laser sights across the screen, then the bullet

#ifndef SNIPERS_RENDER_HPP
    #define SNIPERS_RENDER_HPP

#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
struct Sprites;
struct Run;

class Snipers {
private:
    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    size_t firing = 0;      // how many were firing last frame: a new shot rings out

public:
    Snipers(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio);

    void reset() { firing = 0; }
    void draw(const std::vector<SniperState> &snipers);
};

#endif
