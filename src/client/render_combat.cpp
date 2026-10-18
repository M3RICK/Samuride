/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include <algorithm>
#include <cmath>

// CYBERPSYCHO: drones, attacks and projectiles.
// Projectiles are drawn from the shooter's position and their age: the server moves them
// at run speed + weapon speed, the shooter at run speed, so the gap only depends on the weapon.

void Renderer::handleShots(GameState &state)
{
    for (const ShotEvent &shot : state.takeShots()) {
        sf::Vector2f muzzle = display_positions[shot.player] + sf::Vector2f(0.9f, 1.0f);

        attack_clocks[shot.player].restart();

        switch (shot.weapon) {
            case Weapon::Blade:
                audio.play(Audio::Sfx::Slash);
                emitParticles(muzzle + sf::Vector2f(1, 0), 10, sf::Color::White, 6, 0.25f, 0);
                break;
            case Weapon::Rockets:
                audio.play(Audio::Sfx::Rocket);
                emitParticles(muzzle, 8, NEON_PINK, 4, 0.3f, 0);
                shots.push_back({shot.player, shot.weapon, sf::Clock()});
                break;
            default:
                audio.play(Audio::Sfx::Gun);
                emitParticles(muzzle, 5, NEON_YELLOW, 3, 0.15f, 0);
                shots.push_back({shot.player, shot.weapon, sf::Clock()});
                break;
        }
    }
}

sf::Vector2f Renderer::shotPosition(const ShotTrail &shot) const
{
    auto shooter = display_positions.find(shot.player);
    sf::Vector2f origin = shooter != display_positions.end() ? shooter->second : sf::Vector2f();
    float travelled = statsOf(shot.weapon).speed * shot.age.getElapsedTime().asSeconds();

    return origin + sf::Vector2f(0.9f + travelled, 1.0f);
}

bool Renderer::isLiveDrone(int x, int y, const std::set<std::pair<int, int>> &destroyed) const
{
    return x >= 0 && y >= 0 && world_map.getTile(x, y) == DRONE_TILE && !destroyed.count({x, y});
}

// A shot ends out of range or on the first live drone, like on the server
void Renderer::renderShots(const std::set<std::pair<int, int>> &destroyed)
{
    std::erase_if(shots, [&](const ShotTrail &shot) {
        sf::Vector2f pos = shotPosition(shot);
        float range = statsOf(shot.weapon).speed * shot.age.getElapsedTime().asSeconds();
        return range > PROJECTILE_RANGE || isLiveDrone(pos.x, pos.y, destroyed);
    });

    for (const ShotTrail &shot : shots) {
        bool rocket = shot.weapon == Weapon::Rockets;
        sf::Color color = rocket ? NEON_PINK : NEON_YELLOW;
        sf::Vector2f pos = shotPosition(shot);
        sf::Vector2f size(tile_size * (rocket ? 0.7f : 0.5f), rocket ? 14 : 6);

        sf::RectangleShape bolt(size);
        bolt.setOrigin(size / 2.f);
        bolt.setPosition(toScreen(pos));
        bolt.setFillColor(color);
        window.draw(bolt, sf::BlendAdd);
        drawHalo(toScreen(pos), tile_size * (rocket ? 0.35f : 0.18f), withAlpha(color, 90));

        if (rocket)
            emitParticles(pos - sf::Vector2f(0.35f, 0), 1, NEON_PINK, 1, 0.25f, 0);
    }
}

// Security drone hovering on its tile, red eye glowing
void Renderer::renderDrone(int x, int y, float time)
{
    sf::Vector2f center = toScreen({x + 0.5f, y + 0.5f + std::sin(time * 3 + x) * 0.08f});
    float pulse = 0.5f + 0.5f * std::sin(time * 8 + x);

    drawHalo(center, tile_size * (0.6f + 0.1f * pulse), withAlpha(NEON_RED, 70));
    drawBacking(center, tile_size * 0.55f, withAlpha(NEON_RED, 150 + 100 * pulse));
    drawDroneSprite(center, tile_size * 1.3f, static_cast<int>(time * 12 + x) % DRONE_FRAMES);
}

// The drone sheet is DRONE_FRAMES frames side by side, scaled up without smoothing
void Renderer::drawDroneSprite(sf::Vector2f center, float width, int frame)
{
    sf::Vector2i frame_size(drone_texture.getSize().x / DRONE_FRAMES, drone_texture.getSize().y);

    if (frame_size.x == 0)
        return;

    sf::Sprite sprite(drone_texture);
    float scale = width / frame_size.x;

    sprite.setTextureRect(sf::IntRect({frame * frame_size.x, 0}, frame_size));
    sprite.setOrigin(sf::Vector2f(frame_size) / 2.f);
    sprite.setScale({scale, scale});
    sprite.setPosition(center);
    window.draw(sprite);
}
