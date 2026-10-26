// Adam Smasher: his hulk on the right edge, his attacks and their warnings, his health bar

#ifndef BOSS_RENDER_HPP
    #define BOSS_RENDER_HPP

#include <SFML/System/Clock.hpp>
#include "../state.hpp"

class Canvas;
class Audio;
struct Run;

class Smasher {
private:
    Canvas &canvas;
    Run &run;
    Audio &audio;
    BossState last;     // last frame's: an attack striking, or him stepping in, is heard once
    // Where he is drawn, tiles ahead of his x: he leaps to where the server puts him rather than blink there
    float shown_x = 0;
    sf::Clock frame_clock;

    void drawBody(const BossState &boss);
    void drawAttack(const BossState &boss);
    void drawHealth(const BossState &boss);

public:
    Smasher(Canvas &canvas, Run &run, Audio &audio);

    void reset() { last = {}; shown_x = 0; }
    void draw(const BossState &boss, bool cyberpsycho);
};

#endif
