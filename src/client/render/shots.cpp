// CYBERPSYCHO attacks: muzzle flashes, slashes, the projectiles in flight, and the ultimates

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

        run.attack_clocks[shot.player].restart();

        switch (shot.weapon) {
            case Weapon::Blade:
                audio.play(Audio::Sfx::Slash);
                particles.emit(muzzle + sf::Vector2f(1, 0), 10, sf::Color::White, 6, 0.25f, 0);
                break;
            case Weapon::Hack:
                audio.play(Audio::Sfx::Hack);
                particles.emit(muzzle, 8, NEON_PURPLE, 3, 0.3f, 0);
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
    for (const UltimateEvent &ultimate : state.takeUltimates())
        startUltimate(ultimate);
}

void Shots::startUltimate(const UltimateEvent &ultimate)
{
    run.attack_clocks[ultimate.player].restart();
    run.shakeAtLeast(0.4f);

    if (ultimate.ultimate == Ultimate::FireWave) {
        audio.play(Audio::Sfx::FireWave);
        run.fire_waves.push_back({ultimate.x, ultimate.x, sf::Clock()});
    } else if (ultimate.ultimate == Ultimate::Nuke) {
        audio.play(Audio::Sfx::Rocket);
        run.nukes.push_back({ultimate.x, false, sf::Clock()});
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

// A wall of fire from floor to ceiling; the zappers it went over are out, like on the server
void Shots::drawFireWaves()
{
    float tile_size = run.view.tile_size;

    for (FireWave &wave : run.fire_waves) {
        float front = wave.front();

        run.burnZappers(wave.burnt_to, front);
        wave.burnt_to = front;

        // Fades out once it reached its range
        float fade = std::clamp(1 - (wave.age.getElapsedTime().asSeconds() - FIRE_WAVE_RANGE / FIRE_WAVE_SPEED) * 3, 0.0f, 1.0f);
        sf::Vector2f top = run.toScreen({front, 0});
        sf::Vector2f bottom = run.toScreen({front, run.floor()});

        for (auto [width, color] : {std::pair{2.2f, NEON_RED}, {1.4f, NEON_ORANGE}, {0.6f, NEON_YELLOW}}) {
            sf::RectangleShape flame({tile_size * width, bottom.y - top.y});
            flame.setPosition({top.x - tile_size * width, top.y});
            flame.setFillColor(withAlpha(color, static_cast<uint8_t>(150 * fade)));
            canvas.draw(flame, sf::BlendAdd);
        }
        if (fade > 0.5f) {
            for (int i = 0; i < 6; i++)
                particles.emit({front - randf(0, 1.5f), randf(0, run.floor())}, 1, i % 2 ? NEON_ORANGE : NEON_YELLOW,
                               4, 0.5f, -6);
        }
    }
    std::erase_if(run.fire_waves, [](const FireWave &wave) {
        return wave.age.getElapsedTime().asSeconds() > FIRE_WAVE_RANGE / FIRE_WAVE_SPEED + 0.4f;
    });
}

// The shell arcs from David's arm to the ground ahead, then a white flash and a dome of fire
void Shots::drawNukes()
{
    static constexpr float BLAST_TIME = 0.8f;
    float tile_size = run.view.tile_size;

    for (Nuke &nuke : run.nukes) {
        float t = nuke.age.getElapsedTime().asSeconds();
        sf::Vector2f impact(nuke.start + NUKE_DISTANCE, run.floor());

        if (t < NUKE_DELAY) {
            float k = t / NUKE_DELAY;
            sf::Vector2f from(nuke.start, run.floor() / 2);
            sf::Vector2f shell = from + (impact - from) * k - sf::Vector2f(0, 3 * std::sin(k * 3.1416f));
            canvas.halo(run.toScreen(shell), tile_size * 0.35f, withAlpha(NEON_YELLOW, 200));
            particles.emit(shell, 2, NEON_ORANGE, 1.5f, 0.3f, 0);
            continue;
        }
        if (!nuke.landed) {
            nuke.landed = true;
            audio.play(Audio::Sfx::Nuke);
            run.shakeAtLeast(0.8f);
            run.burnZappers(impact.x - NUKE_RADIUS, impact.x + NUKE_RADIUS);
            particles.emit(impact - sf::Vector2f(0, 2), 80, NEON_ORANGE, 14, 0.9f, -4);
            particles.emit(impact - sf::Vector2f(0, 2), 40, NEON_YELLOW, 10, 0.7f, -4);
        }

        // Swells over the whole razed stretch, then fades
        float k = std::min(1.0f, (t - NUKE_DELAY) / BLAST_TIME);
        auto alpha = static_cast<uint8_t>(220 * (1 - k));
        canvas.halo(run.toScreen(impact), tile_size * NUKE_RADIUS * (0.4f + 0.6f * k), withAlpha(NEON_ORANGE, alpha));
        canvas.halo(run.toScreen(impact), tile_size * NUKE_RADIUS * 0.5f * (0.4f + 0.6f * k), withAlpha(sf::Color::White, alpha));
    }
    std::erase_if(run.nukes, [](const Nuke &nuke) {
        return nuke.age.getElapsedTime().asSeconds() > NUKE_DELAY + BLAST_TIME;
    });
}

// A shot ends out of range, or when the server says it hit (land)
void Shots::draw()
{
    drawFireWaves();
    drawNukes();

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
