// Arasaka snipers: their laser sights across the screen, then the railgun shot

#ifndef SNIPERS_RENDER_HPP
    #define SNIPERS_RENDER_HPP

#include <string>
#include <vector>
#include "../state.hpp"

class Canvas;
class Audio;
struct Run;

// Playing the Hunter with a bullet loaded: where he can parry from, and what to tell him
struct ParryCue {
    float line = -1;        // the middle of his body, -1 when he can't parry
    bool aiming = false;    // holding the attack key long enough
    std::string key;        // its name, for "HOLD [F]"
};

class Snipers {
private:
    Canvas &canvas;
    Run &run;
    Audio &audio;
    size_t firing = 0;      // how many were firing last frame: a new shot rings out

public:
    Snipers(Canvas &canvas, Run &run, Audio &audio);

    void reset() { firing = 0; }
    void draw(const std::vector<SniperState> &snipers, const ParryCue &parry);
};

#endif
