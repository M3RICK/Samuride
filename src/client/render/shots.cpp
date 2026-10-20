// CYBERPSYCHO attacks: muzzle flashes, slashes and the projectiles in flight (the ultimates: ultimates.cpp)

#include "shots.hpp"
#include "canvas.hpp"
#include "particles.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include <algorithm>
#include <cmath>

using namespace Palette;

Shots::Shots(Canvas &canvas, Run &run, Audio &audio, Particles &particles)
    : canvas(canvas), run(run), audio(audio), particles(particles) {
}

void Shots::handle(GameState &state)
{
    for (const ShotEvent &shot : state.takeShots()) {
        sf::Vector2f muzzle = run.display_positions[shot.player] + sf::Vector2f(0.9f, 1.0f);

        // Pow-Pow fires faster than the attack animation: it plays through instead of restarting
        sf::Clock &attack = run.attack_clocks[shot.player];
        if (shot.weapon != Weapon::Minigun || attack.getElapsedTime().asSeconds() > 0.3f)
            attack.restart();

        switch (shot.weapon) {
            case Weapon::Blade:
                audio.play(Audio::Sfx::Slash);
                particles.emit(muzzle + sf::Vector2f(1, 0), 10, sf::Color::White, 6, 0.25f, 0);
                break;
            case Weapon::Hack:
                audio.play(Audio::Sfx::Hack);
                particles.emit(muzzle, 8, NEON_PURPLE, 3, 0.3f, 0);
                break;
            case Weapon::Spear:
                audio.play(Audio::Sfx::Rocket);
                particles.emit(muzzle, 8, sf::Color::White, 4, 0.3f, 0);
                trails.push_back({shot.player, shot.weapon, muzzle.y, sf::Clock()});
                break;
            case Weapon::Minigun:
                audio.play(Audio::Sfx::Minigun);
                particles.emit(muzzle, 3, NEON_PINK, 3, 0.1f, 0);
                trails.push_back({shot.player, shot.weapon, muzzle.y + randf(-0.08f, 0.08f), sf::Clock()});
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

// The player's shot drawn closest to where the server says one hit, if one is close enough: a blast
// that also downs a drone, or an ultimate's kills, must not take another shot still flying
void Shots::land(int player, float x)
{
    static constexpr float MAX_GAP = 4;     // tiles: what the network lag can put between the two
    auto closest = trails.end();
    float gap = MAX_GAP;

    for (auto it = trails.begin(); it != trails.end(); ++it) {
        float distance = std::abs(positionOf(*it).x - x);
        if (it->player == player && distance < gap) {
            gap = distance;
            closest = it;
        }
    }
    if (closest != trails.end())
        trails.erase(closest);
}

// A shot ends out of range, or when the server says it hit (land)
void Shots::draw()
{
    float tile_size = run.view.tile_size;

    std::erase_if(trails, [](const Trail &trail) {
        return statsOf(trail.weapon).speed * trail.age.getElapsedTime().asSeconds() > PROJECTILE_RANGE;
    });

    for (const Trail &trail : trails) {
        bool minigun = trail.weapon == Weapon::Minigun;
        bool spear = trail.weapon == Weapon::Spear;
        sf::Color color = minigun ? NEON_PINK : spear ? sf::Color(200, 250, 255) : NEON_YELLOW;
        sf::Vector2f pos = positionOf(trail);
        sf::Vector2f size(tile_size * (minigun ? 0.35f : spear ? 1.3f : 0.5f), spear ? 8 : 5);

        sf::RectangleShape bolt(size);
        bolt.setOrigin(size / 2.f);
        bolt.setPosition(run.toScreen(pos));
        bolt.setFillColor(color);
        canvas.draw(bolt, sf::BlendAdd);
        canvas.halo(run.toScreen(pos), tile_size * 0.18f, withAlpha(color, 90));
        if (spear)
            particles.emit(pos - sf::Vector2f(0.65f, 0), 1, NEON_CYAN, 1, 0.2f, 0);
    }
}
