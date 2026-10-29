// Adam Smasher: his sprite on the right edge, his attacks and their warnings, his health bar

#ifndef BOSS_RENDER_HPP
    #define BOSS_RENDER_HPP

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Clock.hpp>
#include "../state.hpp"

class Canvas;
class Audio;
struct Run;
struct Sprites;
struct Animation;

class Smasher {
private:
    static constexpr float HEIGHT = 6;      // tiles, feet to head
    static constexpr float LEAVE_CROUCH = 0.4f;     // seconds crouching before he leaps away

    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    BossState last;     // last frame's: an attack striking, him stepping in or taking a hit, is seen once
    // Where he is drawn, tiles ahead of the camera: he leaps to where the server puts him rather than blink there
    float shown_x = 0;
    sf::Clock frame_clock;
    sf::Clock since_entrance;
    sf::Clock since_hit;
    // Once downed, he dies where he stood, left behind: world x, and since when
    float death_x = -1;
    sf::Clock since_death;
    // Still standing at the arena's end, he leaps away off the top: tiles ahead of the camera, and since when
    float leave_x = -1;
    sf::Clock since_leave;

    void react(const BossState &boss);
    // The animation and frame his state shows
    std::pair<const Animation *, int> pose(const BossState &boss) const;
    void drawBody(const Animation &anim, int frame, float x, float lift, sf::Color color);
    void drawAttack(const BossState &boss);
    void drawHealth(const BossState &boss);

public:
    Smasher(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio);

    void reset() { last = {}; shown_x = 0; death_x = -1; leave_x = -1; }
    void draw(const BossState &boss, bool cyberpsycho);
};

#endif
