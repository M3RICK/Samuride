// Arasaka snipers: their laser sights across the screen, then the railgun shot.
// ponytail: plain shapes until the sniper gets its sprite and sound

#include "snipers.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "../audio.hpp"

using namespace Palette;

Snipers::Snipers(Canvas &canvas, Run &run, Audio &audio) : canvas(canvas), run(run), audio(audio) {
}

// Tracking: a thin red line and a reticle on the right edge, following you. Aiming: the line
// thickens and flickers, frozen. Firing: a white-hot beam with a red glow, the whole row.
void Snipers::draw(const std::vector<SniperState> &snipers)
{
    float tile = run.view.tile_size;
    size_t now_firing = 0;

    for (const SniperState &sniper : snipers) {
        float y = run.toScreen({0, sniper.y}).y;

        if (sniper.phase == SniperPhase::Firing) {
            now_firing++;
            for (auto [height, color] : {std::pair{tile * SNIPER_BEAM_HALF * 4, withAlpha(NEON_RED, 90)},
                                         std::pair{tile * SNIPER_BEAM_HALF * 2, NEON_RED},
                                         std::pair{tile * SNIPER_BEAM_HALF, sf::Color::White}}) {
                sf::RectangleShape beam({Screen::WIDTH, height});
                beam.setPosition({0, y - height / 2});
                beam.setFillColor(color);
                canvas.draw(beam);
            }
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
