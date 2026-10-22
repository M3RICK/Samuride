// Arasaka snipers: their laser sights across the screen, then the railgun shot

#ifndef SNIPERS_RENDER_HPP
    #define SNIPERS_RENDER_HPP

#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
struct Run;

class Snipers {
private:
    Canvas &canvas;
    Run &run;
    Audio &audio;
    size_t firing = 0;      // how many were firing last frame: a new shot rings out

public:
    Snipers(Canvas &canvas, Run &run, Audio &audio);

    void reset() { firing = 0; }
    void draw(const std::vector<SniperState> &snipers);
};

#endif
