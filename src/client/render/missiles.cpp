// Missiles flying in from the right; a warning on the right edge follows the one about to come in.
// ponytail: plain shapes until the missile gets its sprite

#include "missiles.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include <cmath>

using namespace Palette;

Missiles::Missiles(Canvas &canvas, Run &run, Audio &audio) : canvas(canvas), run(run), audio(audio) {
}

// A grey body with a red nose pointing left, a flickering flame behind
void Missiles::draw(const std::vector<MissileState> &missiles)
{
    float tile = run.view.tile_size;
    size_t now_on_screen = 0;

    for (const MissileState &missile : missiles) {
        sf::Vector2f tip = run.toScreen({missile.x, missile.y});
        if (tip.x > Screen::WIDTH)
            continue;
        now_on_screen++;

        float half = tile * MISSILE_RADIUS;
        float length = tile * 1.4f;

        sf::ConvexShape nose(3);
        nose.setPoint(0, {0, 0});
        nose.setPoint(1, {half * 1.5f, -half});
        nose.setPoint(2, {half * 1.5f, half});
        nose.setPosition(tip);
        nose.setFillColor(NEON_RED);
        canvas.draw(nose);

        sf::RectangleShape body({length, half * 2});
        body.setPosition(tip + sf::Vector2f(half * 1.5f, -half));
        body.setFillColor(sf::Color(170, 170, 185));
        canvas.draw(body);

        float flicker = 0.7f + 0.3f * std::sin(canvas.time() * 40);
        sf::ConvexShape flame(3);
        flame.setPoint(0, {0, -half * 0.8f});
        flame.setPoint(1, {length * 0.8f * flicker, 0});
        flame.setPoint(2, {0, half * 0.8f});
        flame.setPosition(tip + sf::Vector2f(half * 1.5f + length, 0));
        flame.setFillColor(NEON_ORANGE);
        canvas.draw(flame);
    }

    if (now_on_screen > on_screen)
        audio.play(Audio::Sfx::Rocket);
    on_screen = now_on_screen;
}

// While a missile is still off screen: a yellow blinking sign on the right edge that follows its line,
// turning solid red once the line is locked
void Missiles::drawWarnings(const std::vector<MissileState> &missiles)
{
    for (const MissileState &missile : missiles) {
        if (run.toScreen({missile.x, 0}).x <= Screen::WIDTH || (!missile.locked && !canvas.blink()))
            continue;

        sf::Color color = missile.locked ? NEON_RED : NEON_YELLOW;
        sf::Vector2f center(Screen::WIDTH - 70, run.toScreen({0, missile.y}).y);

        sf::CircleShape sign(45, 3);
        sign.setOrigin({45, 45});
        sign.setPosition(center);
        sign.setRotation(sf::degrees(-90));    // pointing left, at the player
        sign.setFillColor(withAlpha(color, 90));
        sign.setOutlineColor(color);
        sign.setOutlineThickness(4);
        canvas.draw(sign);

        canvas.text("!", 56, sf::Color::White, center);
    }
}
