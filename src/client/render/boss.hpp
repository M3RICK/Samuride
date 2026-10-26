// Adam Smasher: his hulk on the right edge, his attacks and their warnings, his health bar

#ifndef BOSS_RENDER_HPP
    #define BOSS_RENDER_HPP

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

    void drawBody(const BossState &boss);
    void drawAttack(const BossState &boss);
    void drawHealth(const BossState &boss);

public:
    Smasher(Canvas &canvas, Run &run, Audio &audio);

    void reset() { last = {}; }
    void draw(const BossState &boss, bool cyberpsycho);
};

#endif
