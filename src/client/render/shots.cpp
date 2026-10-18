// CYBERPSYCHO attacks: muzzle flashes, slashes and the projectiles in flight

#include "shots.hpp"
#include "canvas.hpp"
#include "particles.hpp"
#include "run.hpp"
#include "../audio.hpp"

using namespace Palette;

Shots::Shots(Canvas &canvas, Run &run, Audio &audio, Particles &particles)
    : canvas(canvas), run(run), audio(audio), particles(particles) {
}

void Shots::handle(GameState &state)
{
    for (const ShotEvent &shot : state.takeShots()) {
        sf::Vector2f muzzle = run.display_positions[shot.player] + sf::Vector2f(0.9f, 1.0f);

        run.attack_clocks[shot.player].restart();

        switch (shot.weapon) {
            case Weapon::Blade:
                audio.play(Audio::Sfx::Slash);
                particles.emit(muzzle + sf::Vector2f(1, 0), 10, sf::Color::White, 6, 0.25f, 0);
                break;
            case Weapon::Rockets:
                audio.play(Audio::Sfx::Rocket);
                particles.emit(muzzle, 8, NEON_PINK, 4, 0.3f, 0);
                trails.push_back({shot.player, shot.weapon, sf::Clock()});
                break;
            default:
                audio.play(Audio::Sfx::Gun);
                particles.emit(muzzle, 5, NEON_YELLOW, 3, 0.15f, 0);
                trails.push_back({shot.player, shot.weapon, sf::Clock()});
                break;
        }
    }
}

sf::Vector2f Shots::positionOf(const Trail &trail) const
{
    auto shooter = run.display_positions.find(trail.player);
    sf::Vector2f origin = shooter != run.display_positions.end() ? shooter->second : sf::Vector2f();
    float travelled = statsOf(trail.weapon).speed * trail.age.getElapsedTime().asSeconds();

    return origin + sf::Vector2f(0.9f + travelled, 1.0f);
}

// A shot ends out of range or on the first live drone, like on the server
void Shots::draw(const std::set<std::pair<int, int>> &destroyed)
{
    float tile_size = run.view.tile_size;

    std::erase_if(trails, [&](const Trail &trail) {
        sf::Vector2f pos = positionOf(trail);
        float range = statsOf(trail.weapon).speed * trail.age.getElapsedTime().asSeconds();
        return range > PROJECTILE_RANGE || run.isLiveDrone(pos.x, pos.y, destroyed);
    });

    for (const Trail &trail : trails) {
        bool rocket = trail.weapon == Weapon::Rockets;
        sf::Color color = rocket ? NEON_PINK : NEON_YELLOW;
        sf::Vector2f pos = positionOf(trail);
        sf::Vector2f size(tile_size * (rocket ? 0.7f : 0.5f), rocket ? 14 : 6);

        sf::RectangleShape bolt(size);
        bolt.setOrigin(size / 2.f);
        bolt.setPosition(run.toScreen(pos));
        bolt.setFillColor(color);
        canvas.draw(bolt, sf::BlendAdd);
        canvas.halo(run.toScreen(pos), tile_size * (rocket ? 0.35f : 0.18f), withAlpha(color, 90));

        if (rocket)
            particles.emit(pos - sf::Vector2f(0.35f, 0), 1, NEON_PINK, 1, 0.25f, 0);
    }
}
