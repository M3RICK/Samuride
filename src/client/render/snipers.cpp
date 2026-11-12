// Arasaka snipers: their laser sights across the screen, then the bullet (Alex's sprite).

#include "snipers.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "sprites.hpp"
#include "../audio.hpp"

using namespace Palette;

Snipers::Snipers(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio) : canvas(canvas), sprites(sprites), run(run), audio(audio) {
}

// Tracking: a thin red line and a reticle on the right edge, following you. Aiming: the line
// thickens and flickers, frozen. Firing: the bullet flies along the row, a faint tracer behind it.
void Snipers::draw(const std::vector<SniperState> &snipers)
{
    float tile = run.view.tile_size;
    size_t now_firing = 0;

    for (const SniperState &sniper : snipers) {
        float y = run.toScreen({0, sniper.y}).y;

        if (sniper.phase == SniperPhase::Firing) {
            now_firing++;
            // Nose on the bullet's x, like the hitbox; the sprite is 3 frames-heights long
            float x = run.toScreen({sniper.x, 0}).x;
            sf::RectangleShape tracer({std::max(0.0f, Screen::WIDTH - x), 3});
            tracer.setPosition({x, y - 1.5f});
            tracer.setFillColor(withAlpha(NEON_RED, 70));
            canvas.draw(tracer);
            canvas.image(sprites.sniper_bullet, {x - tile * 0.25f, y}, tile, {0, 0.5f}, sf::Color::White);
            continue;
        }

        bool aiming = sniper.phase == SniperPhase::Aiming;
        sf::Color color = NEON_RED;
        if (aiming && !canvas.blink())
            continue;

        float thickness = aiming ? 6 : 2;
        sf::RectangleShape line({Screen::WIDTH, thickness});
        line.setPosition({0, y - thickness / 2});
        line.setFillColor(withAlpha(color, aiming ? 230 : 140));
        canvas.draw(line);

        sf::Vector2f center(Screen::WIDTH - 70, y);
        sf::CircleShape reticle(36);
        reticle.setOrigin({36, 36});
        reticle.setPosition(center);
        reticle.setFillColor(sf::Color::Transparent);
        reticle.setOutlineColor(color);
        reticle.setOutlineThickness(4);
        canvas.draw(reticle);

        sf::RectangleShape cross({4, 96});
        cross.setOrigin({2, 48});
        cross.setPosition(center);
        cross.setFillColor(color);
        canvas.draw(cross);
    }

    if (now_firing > firing) {
        audio.play(Audio::Sfx::Gun);
        run.shakeAtLeast(0.2f);
    }
    firing = now_firing;
}
