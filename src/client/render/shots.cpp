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
    } else if (ultimate.ultimate == Ultimate::Fishbones) {
        audio.play(Audio::Sfx::Rocket);
        run.rockets.push_back({ultimate.x, ultimate.y, ultimate.x, false, sf::Clock()});
    } else if (ultimate.ultimate == Ultimate::ThunderSpears) {
        audio.play(Audio::Sfx::Thunder);
        particles.emit(run.display_positions[ultimate.player] + sf::Vector2f(0.5f, 1), 30, NEON_CYAN, 6, 0.5f, 0);
    } else if (ultimate.ultimate == Ultimate::Breach) {
        audio.play(Audio::Sfx::Breach);
        run.hackZappers(ultimate.x - 2, ultimate.x + BREACH_AHEAD, BREACH_TIME);
        run.breaches.emplace_back();
        // Sparks on the zappers it just turned off
        for (const auto &[node, hack] : run.hacked_zappers) {
            if (run.hackLeft(node.first, node.second) > BREACH_TIME - 0.1f)
                particles.emit({node.first + 0.5f, node.second + 0.5f}, 6, NEON_PURPLE, 4, 0.5f, 0);
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

// A big shark rocket along Jinx's line, leaving a razed pink band behind, and a blast at the end
void Shots::drawRockets()
{
    static constexpr float FADE_TIME = 0.6f;
    float tile_size = run.view.tile_size;

    for (Rocket &rocket : run.rockets) {
        float front = rocket.front();
        auto [top, bottom] = fishbonesRows(rocket.y);
        float flight = FISHBONES_RANGE / FISHBONES_SPEED;
        float t = rocket.age.getElapsedTime().asSeconds();
        float fade = std::clamp(1 - (t - flight) / FADE_TIME, 0.0f, 1.0f);

        run.burnZappers(rocket.burnt_to, front, top, bottom);
        rocket.burnt_to = front;

        // The band it cleared
        sf::Vector2f from = run.toScreen({rocket.start, static_cast<float>(top)});
        sf::Vector2f to = run.toScreen({front, static_cast<float>(bottom + 1)});
        sf::RectangleShape band(to - from);
        band.setPosition(from);
        band.setFillColor(withAlpha(NEON_PINK, static_cast<uint8_t>(35 * fade)));
        canvas.draw(band, sf::BlendAdd);

        if (t < flight) {
            sf::Vector2f nose = run.toScreen({front, rocket.y});
            sf::RectangleShape body({tile_size * 2.4f, tile_size * 0.8f});
            body.setOrigin({body.getSize().x, body.getSize().y / 2});
            body.setPosition(nose);
            body.setFillColor(NEON_PINK);
            canvas.draw(body);
            canvas.halo(nose, tile_size * 0.9f, withAlpha(NEON_PINK, 120));
            particles.emit({front - 2.4f, rocket.y}, 4, NEON_ORANGE, 3, 0.4f, 0);
        } else if (!rocket.blown) {
            rocket.blown = true;
            audio.play(Audio::Sfx::Explosion);
            run.shakeAtLeast(0.4f);
            particles.emit({front, rocket.y}, 60, NEON_PINK, 10, 0.7f, 0);
            particles.emit({front, rocket.y}, 30, NEON_ORANGE, 8, 0.6f, 0);
        }
    }
    std::erase_if(run.rockets, [](const Rocket &rocket) {
        return rocket.age.getElapsedTime().asSeconds() > FISHBONES_RANGE / FISHBONES_SPEED + FADE_TIME;
    });
}

// The whole screen glitches purple for a moment
void Shots::drawBreaches()
{
    static constexpr float GLITCH_TIME = 0.7f;

    for (const sf::Clock &breach : run.breaches) {
        float t = breach.getElapsedTime().asSeconds() / GLITCH_TIME;
        auto alpha = static_cast<uint8_t>(110 * (1 - t));

        sf::RectangleShape tint({Screen::WIDTH, Screen::HEIGHT});
        tint.setFillColor(withAlpha(NEON_PURPLE, alpha / 2));
        canvas.draw(tint, sf::BlendAdd);
        for (int i = 0; i < 8; i++) {
            sf::RectangleShape band({Screen::WIDTH, randf(4, 30)});
            band.setPosition({randf(-60, 60), randf(0, Screen::HEIGHT)});
            band.setFillColor(withAlpha(i % 2 ? NEON_PURPLE : NEON_CYAN, alpha));
            canvas.draw(band, sf::BlendAdd);
        }
        canvas.glitchText("SYSTEM BREACH", 110, withAlpha(NEON_PURPLE, static_cast<uint8_t>(255 * (1 - t))),
                          Screen::CENTER);
    }
    std::erase_if(run.breaches, [](const sf::Clock &breach) { return breach.getElapsedTime().asSeconds() > GLITCH_TIME; });
}

// A shot ends out of range, or when the server says it hit (land)
void Shots::draw()
{
    drawFireWaves();
    drawNukes();
    drawBreaches();
    drawRockets();

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
