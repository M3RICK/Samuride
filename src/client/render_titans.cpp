#include "render.hpp"
#include <cmath>

// Titans run along the floor toward the player; a warning shows where one is about to burst in

void Renderer::renderTitans(const std::vector<TitanState> &titans)
{
    float time = animation_clock.getElapsedTime().asSeconds();

    for (const TitanState &titan : titans)
        drawTitan(titan, time);
}

// Feet on the floor, scaled to the titan's height in tiles
void Renderer::drawTitan(const TitanState &titan, float time)
{
    const Character &sprite = titan_sprites[static_cast<int>(titan.type)];
    TitanStats stats = titanStats(titan.type);

    if (sprite.sheet.getSize().x == 0)
        return;

    sf::Vector2f feet = toScreen({titan.x + stats.width / 2, static_cast<float>(world_map.getHeight())});
    float scale = tile_size * stats.height / sprite.body;

    sf::Sprite body(sprite.sheet);
    body.setTextureRect(sprite.frameRect(sprite.run, sprite.run.frameAt(time)));
    body.setOrigin({sprite.frame_size.x / 2.f, sprite.feet});
    body.setScale({scale, scale});
    body.setPosition(feet);
    window.draw(body);
}

// A blinking "!" on the right edge, at the titan's height, while it's still off screen
void Renderer::renderTitanWarnings(const std::vector<TitanState> &titans)
{
    if (!blink())
        return;

    for (const TitanState &titan : titans) {
        TitanStats stats = titanStats(titan.type);
        float left = toScreen({titan.x, 0}).x;

        if (left <= SCREEN_WIDTH)
            continue;

        float floor = world_map.getHeight();
        sf::Vector2f pos(SCREEN_WIDTH - 70, toScreen({0, floor - stats.height / 2}).y);
        drawHalo(pos, 55, withAlpha(NEON_RED, 120));
        drawGlitchText("!", 90, sf::Color::White, pos);
        window.draw(makeText("TITAN", 24, NEON_RED, pos + sf::Vector2f(0, 70)));
    }
}

// Each titan that bursts onto the screen lands with a stomp
void Renderer::updateTitanStomps(const std::vector<TitanState> &titans)
{
    size_t on_screen = 0;

    for (const TitanState &titan : titans) {
        if (toScreen({titan.x, 0}).x <= SCREEN_WIDTH)
            on_screen++;
    }

    if (on_screen > titans_on_screen) {
        audio.play(Audio::Sfx::Stomp);
        shake = std::max(shake, 0.25f);
    }
    titans_on_screen = on_screen;
}
