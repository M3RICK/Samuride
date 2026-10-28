// Map tiles: coins, zappers, CYBERPSYCHO drones and Attack Titan orbs

#include "tiles.hpp"
#include "canvas.hpp"
#include "sprites.hpp"
#include "run.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace Palette;

Tiles::Tiles(Canvas &canvas, const Sprites &sprites, const Run &run)
    : canvas(canvas), sprites(sprites), run(run) {
}

void Tiles::draw(const std::set<std::pair<int, int>> &collected, const std::set<std::pair<int, int>> &destroyed,
                 const std::set<std::pair<int, int>> &pinata)
{
    const WorldView &view = run.view;
    int start_x = std::max(0, static_cast<int>(view.camera_x) - 1);
    int end_x = std::min(static_cast<int>(run.map.getWidth()),
                         static_cast<int>(view.camera_x + Screen::WIDTH / view.tile_size) + 2);
    float time = canvas.time();

    for (size_t y = 0; y < run.map.getHeight(); y++) {
        for (int x = start_x; x < end_x; x++) {
            char tile = run.map.getTile(x, y);

            // A destroyed drone may have left a power-up behind
            if (tile == DRONE_TILE && destroyed.count({x, y}))
                tile = droneDrop(x, y, pinata.count({x, y}));

            if (tile == ZAPPER_TILE && run.burnt_zappers.count({x, y}))
                drawSprite(sprites.zapper, {static_cast<float>(x), static_cast<float>(y)}, 0.5f, sf::Color(70, 50, 45, 170));
            else if (tile == ZAPPER_TILE && run.hackLeft(x, y) > 0)
                drawHackedZapper(x, y, run.hackLeft(x, y));
            else if (tile == ZAPPER_TILE)
                drawZapper(x, y, time);
            else if (tile == DRONE_TILE)
                drawDrone(x, y, time);
            else if (collected.count({x, y}))
                continue;
            else if (tile == COIN_TILE)
                drawCoin(x, y, time);
            else if (tile == SHIFT_TILE)
                drawShiftOrb(x, y, time);
            else if (isPowerUp(tile))
                drawPowerUp(x, y, tile, time);
        }
    }
}

// Bobbing, spinning coin on a dark disc ringed in yellow: the pink coin alone vanishes on neon skylines
void Tiles::drawCoin(int x, int y, float time)
{
    float tile_size = run.view.tile_size;
    sf::Vector2f size(sprites.coin.getSize());
    sf::Vector2f center = run.toScreen({x + 0.5f, y + 0.5f + std::sin(time * 4 + x) * 0.06f});

    canvas.halo(center, tile_size * 0.45f, withAlpha(NEON_YELLOW, 45));
    canvas.backing(center, tile_size * 0.36f, withAlpha(NEON_YELLOW, 220));

    if (size.y == 0)
        return;

    float scale = tile_size * 0.6f / size.y;
    float spin = std::max(0.25f, std::abs(std::cos(time * 3 + x * 0.7f)));
    sf::Sprite coin(sprites.coin);
    coin.setOrigin(size / 2.f);
    coin.setScale({scale * spin, scale});
    coin.setPosition(center);
    canvas.draw(coin);
}

// Jetpack Joyride zapper: neighbouring nodes are linked by a live beam
void Tiles::drawZapper(int x, int y, float time)
{
    static const sf::Vector2i links[] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    float tile_size = run.view.tile_size;
    sf::Vector2f center = run.toScreen({x + 0.5f, y + 0.5f});
    bool linked = false;

    for (sf::Vector2i link : links) {
        if (y + link.y >= 0 && run.map.getTile(x + link.x, y + link.y) == ZAPPER_TILE) {
            drawBeam(center, run.toScreen({x + link.x + 0.5f, y + link.y + 0.5f}));
            linked = true;
        }
    }

    // A lone node just crackles
    for (int i = 0; !linked && i < 3; i++) {
        float angle = randf(0, 6.2832f);
        sf::Vector2f tip = center + sf::Vector2f(std::cos(angle), std::sin(angle)) * tile_size * randf(0.4f, 0.7f);
        drawLightning(center, tip, i == 0 ? sf::Color::White : NEON_CYAN);
    }

    float pulse = 0.8f + 0.2f * std::sin(time * 14 + x);
    canvas.halo(center, tile_size * 0.5f * pulse, withAlpha(NEON_CYAN, 70));

    sf::Color flicker(255, 255, 255, std::rand() % 8 ? 255 : 140);
    drawSprite(sprites.zapper, {static_cast<float>(x), static_cast<float>(y)}, 0.55f, flicker);
}

// Lucy hacked it: a dead purple node, blinking just before the beams come back
void Tiles::drawHackedZapper(int x, int y, float left)
{
    bool waking = left < 0.3f && std::rand() % 2;
    sf::Color color = waking ? sf::Color::White : withAlpha(NEON_PURPLE, std::rand() % 8 ? 120 : 200);

    drawSprite(sprites.zapper, {static_cast<float>(x), static_cast<float>(y)}, 0.55f, color);
}

void Tiles::drawBeam(sf::Vector2f from, sf::Vector2f to)
{
    sf::Vector2f delta = to - from;
    float flicker = randf(0.7f, 1);

    sf::RectangleShape glow({delta.length(), run.view.tile_size * 0.35f * flicker});
    glow.setOrigin({0, glow.getSize().y / 2});
    glow.setPosition(from);
    glow.setRotation(delta.angle());
    glow.setFillColor(withAlpha(NEON_CYAN, 60));
    canvas.draw(glow, sf::BlendAdd);

    sf::RectangleShape core({delta.length(), 4});
    core.setOrigin({0, 2});
    core.setPosition(from);
    core.setRotation(delta.angle());
    core.setFillColor(withAlpha(sf::Color::White, 200 * flicker));
    canvas.draw(core, sf::BlendAdd);

    drawLightning(from, to, NEON_CYAN);
    drawLightning(from, to, sf::Color::White);
}

void Tiles::drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color)
{
    const int segments = 6;
    sf::Vector2f normal = (to - from).perpendicular().normalized();
    sf::VertexArray bolt(sf::PrimitiveType::LineStrip);

    for (int i = 0; i <= segments; i++) {
        float jitter = (i == 0 || i == segments) ? 0 : randf(-0.15f, 0.15f) * run.view.tile_size;
        bolt.append({from + (to - from) * (i / static_cast<float>(segments)) + normal * jitter, color});
    }
    canvas.draw(bolt, sf::BlendAdd);

    // Lines are 1px, a second pass makes the arc readable once the window is scaled down
    for (size_t i = 0; i < bolt.getVertexCount(); i++)
        bolt[i].position += {1.5f, 1.5f};
    canvas.draw(bolt, sf::BlendAdd);
}

// Security drone hovering on its tile, red eye glowing
void Tiles::drawDrone(int x, int y, float time)
{
    float tile_size = run.view.tile_size;
    sf::Vector2f center = run.toScreen({x + 0.5f, y + 0.5f + std::sin(time * 3 + x) * 0.08f});
    float pulse = 0.5f + 0.5f * std::sin(time * 8 + x);
    int frame = static_cast<int>(time * 12 + x) % Sprites::DRONE_FRAMES;

    canvas.halo(center, tile_size * (0.6f + 0.1f * pulse), withAlpha(NEON_RED, 70));
    canvas.backing(center, tile_size * 0.55f, withAlpha(NEON_RED, 150 + 100 * pulse));
    canvas.stripFrame(sprites.drone, Sprites::DRONE_FRAMES, frame, center, tile_size * 1.3f);
}

// Orb with the titan's face, on an orange-ringed disc
void Tiles::drawShiftOrb(int x, int y, float time)
{
    float tile_size = run.view.tile_size;
    sf::Vector2f center = run.toScreen({x + 0.5f, y + 0.5f + std::sin(time * 3 + x) * 0.1f});
    float pulse = 0.5f + 0.5f * std::sin(time * 6);

    canvas.halo(center, tile_size * (0.7f + 0.15f * pulse), withAlpha(NEON_ORANGE, 70));
    canvas.backing(center, tile_size * 0.45f, withAlpha(NEON_ORANGE, 230));
    canvas.image(sprites.titan_face, center, tile_size * 0.7f, {0.5f, 0.5f}, sf::Color::White);
}

// Shield: a hexagon plate; Sandevistan: chevrons rushing forward; Berserk: crossed blades
void Tiles::drawPowerUp(int x, int y, char tile, float time)
{
    float tile_size = run.view.tile_size;
    bool shield = tile == SHIELD_TILE;
    sf::Color color = powerUpColor(tile);
    sf::Vector2f center = run.toScreen({x + 0.5f, y + 0.5f + std::sin(time * 3 + x) * 0.1f});
    float pulse = 0.5f + 0.5f * std::sin(time * 6 + x);

    canvas.halo(center, tile_size * (0.65f + 0.1f * pulse), withAlpha(color, 70));
    canvas.backing(center, tile_size * 0.45f, withAlpha(color, 230));

    if (shield) {
        canvas.hexagon(center, tile_size * 0.26f, color, time * 40);
        return;
    }
    if (tile == BERSERK_TILE) {
        sf::RectangleShape blade({tile_size * 0.55f, tile_size * 0.09f});
        blade.setOrigin(blade.getSize() / 2.f);
        blade.setPosition(center);
        blade.setFillColor(color);
        for (float angle : {45.f, -45.f}) {
            blade.setRotation(sf::degrees(angle));
            canvas.draw(blade);
        }
        return;
    }

    sf::ConvexShape chevron(6);
    float s = tile_size * 0.13f;
    chevron.setPoint(0, {-s, -1.6f * s});
    chevron.setPoint(1, {0, -1.6f * s});
    chevron.setPoint(2, {1.4f * s, 0});
    chevron.setPoint(3, {0, 1.6f * s});
    chevron.setPoint(4, {-s, 1.6f * s});
    chevron.setPoint(5, {0.4f * s, 0});
    chevron.setFillColor(color);
    for (float dx : {-0.8f, 0.6f}) {
        chevron.setPosition(center + sf::Vector2f(dx * s, 0));
        canvas.draw(chevron);
    }
}

void Tiles::drawSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color)
{
    sf::Vector2f size(texture.getSize());

    if (size.x == 0)
        return;

    sf::Sprite sprite(texture);
    float scale = run.view.tile_size * fill / std::max(size.x, size.y);

    sprite.setOrigin(size / 2.f);
    sprite.setScale({scale, scale});
    sprite.setPosition(run.toScreen(world + sf::Vector2f(0.5f, 0.5f)));
    sprite.setColor(color);
    canvas.draw(sprite);
}
