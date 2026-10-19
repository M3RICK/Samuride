// Titans running along the floor toward the player; a warning shows where one is about to burst in

#include "titans.hpp"
#include "canvas.hpp"
#include "sprites.hpp"
#include "run.hpp"
#include "../audio.hpp"

using namespace Palette;

Titans::Titans(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio)
    : canvas(canvas), sprites(sprites), run(run), audio(audio) {
}

// Each titan that bursts onto the screen lands with a stomp
void Titans::draw(const std::vector<TitanState> &titans)
{
    float time = canvas.time();
    size_t now_on_screen = 0;

    for (const TitanState &titan : titans) {
        if (run.toScreen({titan.x, 0}).x <= Screen::WIDTH)
            now_on_screen++;
        drawTitan(titan, time);
    }

    if (now_on_screen > on_screen) {
        audio.play(Audio::Sfx::Stomp);
        run.shakeAtLeast(0.25f);
    }
    on_screen = now_on_screen;
}

// Feet on the floor, scaled to the titan's height in tiles
void Titans::drawTitan(const TitanState &titan, float time)
{
    const Character &sprite = sprites.titans[static_cast<int>(titan.type)];
    TitanStats stats = titanStats(titan.type);

    if (sprite.sheet.getSize().x == 0)
        return;

    sf::Vector2f feet = run.toScreen({titan.x + stats.width / 2, run.floor()});
    float scale = run.view.tile_size * stats.height / sprite.body;

    sf::Sprite body(sprite.sheet);
    body.setTextureRect(sprite.frameRect(sprite.run, sprite.run.frameAt(time)));
    body.setOrigin({sprite.frame_size.x / 2.f, sprite.feet});
    body.setScale({scale, scale});
    body.setPosition(feet);
    canvas.draw(body);
}

// While a titan is still off screen, a blinking red column as tall as it stands on the right
// edge, where it will burst in
void Titans::drawWarnings(const std::vector<TitanState> &titans)
{
    if (!canvas.blink())
        return;

    for (const TitanState &titan : titans) {
        TitanStats stats = titanStats(titan.type);

        if (run.toScreen({titan.x, 0}).x <= Screen::WIDTH)
            continue;

        float top = run.toScreen({0, run.floor() - stats.height}).y;
        float bottom = run.toScreen({0, run.floor()}).y;
        sf::Vector2f center(Screen::WIDTH - 80, (top + bottom) / 2);

        sf::RectangleShape column({140, bottom - top});
        column.setOrigin({70, 0});
        column.setPosition({center.x, top});
        column.setFillColor(withAlpha(NEON_RED, 70));
        column.setOutlineColor(NEON_RED);
        column.setOutlineThickness(4);
        canvas.draw(column);

        canvas.glitchText("!", (bottom - top) * 0.6f, sf::Color::White, center - sf::Vector2f(0, 30));
        canvas.text("TITAN", 30, sf::Color::White, {center.x, bottom - 35});
    }
}
