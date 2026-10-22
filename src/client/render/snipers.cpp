// Arasaka snipers: their laser sights across the screen, then the railgun shot.
// ponytail: plain shapes until the sniper gets its sprite and sound

#include "snipers.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include "../touch.hpp"

using namespace Palette;

Snipers::Snipers(Canvas &canvas, Run &run, Audio &audio) : canvas(canvas), run(run), audio(audio) {
}

// Tracking: a thin red line and a reticle on the right edge, following you. Aiming: the line
// thickens and flickers, frozen. Firing: a white-hot beam with a red glow, the whole row.
// Playing the Hunter, the reticle on your line coaches the parry: "HOLD" while it tracks you, then frozen
// while you aim it turns quicksilver, stops flickering and says "RELEASE".
void Snipers::draw(const std::vector<SniperState> &snipers, const ParryCue &parry)
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
        bool on_you = parry.line >= 0 && parryReaches(sniper.y, parry.line);
        bool release = aiming && on_you && parry.aiming;
        sf::Color color = release ? QUICKSILVER : NEON_RED;
        if (aiming && !release && !canvas.blink())
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

        if (release)
            canvas.glitchText("RELEASE!", 40, QUICKSILVER, center - sf::Vector2f(150, 0));
        else if (on_you && !aiming)
            canvas.text(Touch::hint(("HOLD [" + parry.key + "]").c_str(), "HOLD ATTACK"), 30,
                        parry.aiming ? QUICKSILVER : DIM_WHITE, center - sf::Vector2f(150, 0));
    }

    if (now_firing > firing) {
        audio.play(Audio::Sfx::Gun);
        run.shakeAtLeast(0.2f);
    }
    firing = now_firing;
}
