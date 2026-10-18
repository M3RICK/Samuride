// CYBERPSYCHO attacks: muzzle flashes, slashes and the projectiles in flight

#include "shots.hpp"
#include "canvas.hpp"
#include "particles.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include <algorithm>

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
                trails.push_back({shot.player, shot.weapon, muzzle.y, sf::Clock()});
                break;
            default:
                audio.play(Audio::Sfx::Gun);
                particles.emit(muzzle, 5, NEON_YELLOW, 3, 0.15f, 0);
                trails.push_back({shot.player, shot.weapon, muzzle.y, sf::Clock()});
                break;
        }
    }
}

sf::Vector2f Shots::positionOf(const Trail &trail) const
{
    auto shooter = run.display_positions.find(trail.player);
    float shooter_x = shooter != run.display_positions.end() ? shooter->second.x : 0;
    float travelled = statsOf(trail.weapon).speed * trail.age.getElapsedTime().asSeconds();

    return {shooter_x + 0.9f + travelled, trail.y};
}

void Shots::land(int player)
{
    auto oldest = std::find_if(trails.begin(), trails.end(), [&](const Trail &trail) { return trail.player == player; });

    if (oldest != trails.end())
        trails.erase(oldest);
}

// A shot ends out of range, or when the server says it hit (land)
void Shots::draw()
{
    float tile_size = run.view.tile_size;

    std::erase_if(trails, [](const Trail &trail) {
        return statsOf(trail.weapon).speed * trail.age.getElapsedTime().asSeconds() > PROJECTILE_RANGE;
    });

    for (const Trail &trail : trails) {
        bool rocket = trail.weapon == Weapon::Rockets;
        sf::Color color = rocket ? NEON_PINK : NEON_YELLOW;
        sf::Vector2f pos = positionOf(trail);
        sf::Vector2f size(tile_size * (rocket ? 1.1f : 0.5f), rocket ? 26 : 6);

        sf::RectangleShape bolt(size);
        bolt.setOrigin(size / 2.f);
        bolt.setPosition(run.toScreen(pos));
        bolt.setFillColor(color);
        canvas.draw(bolt, sf::BlendAdd);
        canvas.halo(run.toScreen(pos), tile_size * (rocket ? 0.6f : 0.18f), withAlpha(color, 90));

        if (rocket)
            particles.emit(pos - sf::Vector2f(0.55f, 0), 2, NEON_PINK, 1.5f, 0.3f, 0);
    }
}
