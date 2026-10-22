// Missiles flying in from the right; a warning on the right edge follows the one about to come in

#ifndef MISSILES_RENDER_HPP
    #define MISSILES_RENDER_HPP

#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
struct Run;

class Missiles {
private:
    Canvas &canvas;
    Run &run;
    Audio &audio;
    size_t on_screen = 0;   // how many were on screen last frame: a new one roars in

public:
    Missiles(Canvas &canvas, Run &run, Audio &audio);

    void reset() { on_screen = 0; }
    void draw(const std::vector<MissileState> &missiles);
    void drawWarnings(const std::vector<MissileState> &missiles);
};

#endif
