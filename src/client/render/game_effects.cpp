// What happens in the run, as the server reports it (MSG_COLLISION): sounds, sparks, shakes, and what
// floats up afterwards

#include "game_screen.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include <algorithm>
#include <cmath>
#include <format>

using namespace Palette;

// Each effect the server reports; the ones that float up afterwards are kept in `effects`
void GameScreen::addEffect(const CollisionEffect &effect, int me)
{
    bool mine = effect.player == me;

    switch (effect.type) {
        case ZAPPER_TILE:
            playerDown(effect, Audio::Sfx::Zap, NEON_CYAN, 0.5f, 18);
            break;
        case TITAN_HIT:
            playerDown(effect, Audio::Sfx::Stomp, NEON_RED, 0.6f, 22);
            break;
        case SNIPER_HIT:
            playerDown(effect, Audio::Sfx::Explosion, NEON_ORANGE, 0.6f, 22);
            break;
        case TITAN_KILL:
            titanDown(effect, mine);
            break;
        case DRONE_TILE:
            droneDown(effect, me);
            break;
        case HACK_BOUNCE:
            hackBounced(effect);
            return;
        case SPEAR_BLAST:
            spearBlast(effect);
            break;
        case ZAPPER_HACK:
            zapperHacked(effect);
            return;
        case SHIELD_BREAK:
            shieldBroken(effect, mine);
            break;
        case SHIFT_TILE:
            // The Attack Titan's orb: the transformation roars and smokes on its own (players.cpp)
            if (mine)
                react(Mood::Pickup);
            return;
        default:
            if (isPowerUp(effect.type)) {
                powerUpTaken(effect, mine);
            } else if (mine) {
                eddyTaken(effect);
            } else {
                return;
            }
    }
    // Dante's drones pay at his style rank
    int bounty = effect.type == TITAN_KILL ? TITAN_BOUNTY : static_cast<int>(std::lround(DRONE_BOUNTY * STYLE_BOUNTY[style_shown]));
    effects.push_back({effect, sf::Clock(), mine, bounty});
}

static sf::Vector2f centerOf(const CollisionEffect &effect)
{
    return {effect.x + 0.5f, effect.y + 0.5f};
}

// Electrocuted, stomped or shot: the run is over for them
void GameScreen::playerDown(const CollisionEffect &effect, Audio::Sfx sound, sf::Color color, float shake, float strength)
{
    audio.play(sound);
    run.shakeAtLeast(shake, strength);
    run.death_clocks.try_emplace(effect.player);
    particles.emit(centerOf(effect), 40, color, 9, 0.7f, 12);
    particles.emit(centerOf(effect), 20, sf::Color::White, 6, 0.5f, 12);
}

// Steam bursting from the nape
void GameScreen::titanDown(const CollisionEffect &effect, bool mine)
{
    sf::Vector2f nape = centerOf(effect) + sf::Vector2f(0, -3);

    if (mine)
        react(Mood::Kill);
    audio.play(Audio::Sfx::Explosion);
    run.shakeAtLeast(0.3f);
    particles.emit(nape, 50, sf::Color(230, 230, 230), 7, 1.0f, -4);
    particles.emit(nape, 20, NEON_RED, 6, 0.6f, 8);
}

void GameScreen::droneDown(const CollisionEffect &effect, int me)
{
    sf::Vector2f center = centerOf(effect);

    audio.play(Audio::Sfx::Explosion);
    drones_down += effect.player == me;
    if (effect.player == me)
        react(Mood::Kill);
    last_drone[effect.player] = center;
    // Mikasa's blade bites: a hard jolt and white sparks; any other kill, a light one
    if (effect.player == me && weaponOf(run.characterOf(me)) == Weapon::Blade) {
        // Felt at the drone, not over the whole screen: she cuts often, the run must keep its rhythm
        hit_stop = HIT_STOP;
        cuts.push_back({center, sf::Clock()});
        run.shakeAtLeast(0.15f, 16);
        particles.emit(center, 16, sf::Color::White, 10, 0.25f, 0);
    } else if (weaponOf(run.characterOf(effect.player)) == Weapon::Hack && run.display_positions.count(effect.player)) {
        // Lucy's hack lands: a bolt from her hand (the bounce brought its own), the drone glitching apart in purple
        if (!bounced.erase(effect.player))
            zaps.push_back({run.display_positions[effect.player] + sf::Vector2f(0.9f, 1.0f), center, sf::Clock()});
        run.shakeAtLeast(0.15f, effect.player == me ? 12 : 4);
        particles.emit(center, 18, NEON_PURPLE, 7, 0.35f, 0);
        particles.emit(center, 8, NEON_CYAN, 5, 0.25f, 0);
    } else {
        run.shakeAtLeast(0.12f, effect.player == me ? 8 : 4);
    }
    shots.land(effect.player, center.x);
    particles.emit(center, 30, NEON_ORANGE, 8, 0.6f, 6);
    particles.emit(center, 15, NEON_RED, 5, 0.5f, 6);
}

// The bolt jumps from the drone Lucy hacked to the next one (its blast comes next)
void GameScreen::hackBounced(const CollisionEffect &effect)
{
    sf::Vector2f to = centerOf(effect);
    sf::Vector2f from = last_drone.count(effect.player) ? last_drone[effect.player] : to;

    zaps.push_back({from, to, sf::Clock()});
    bounced.insert(effect.player);
}

void GameScreen::spearBlast(const CollisionEffect &effect)
{
    audio.play(Audio::Sfx::Thunder);
    run.shakeAtLeast(0.35f);
    shots.land(effect.player, centerOf(effect).x);
    run.burnZappers(effect.x - SPEAR_BLAST_RADIUS, effect.x + SPEAR_BLAST_RADIUS);
    for (int i = 0; i < 4; i++)
        particles.emit({effect.x + 0.5f, randf(0, run.floor())}, 10, i % 2 ? NEON_CYAN : sf::Color::White, 6, 0.5f, 0);
}

// The whole zapper goes off: sparks on its nodes, nothing drawn over the screen (the shot plays the sound)
void GameScreen::zapperHacked(const CollisionEffect &effect)
{
    for (std::pair<int, int> node : zapperNodes(run.map, effect.x, effect.y)) {
        run.hacked_zappers[node] = {sf::Clock(), HACK_TIME};
        particles.emit({node.first + 0.5f, node.second + 0.5f}, 10, NEON_PURPLE, 4, 0.4f, 0);
    }
}

void GameScreen::shieldBroken(const CollisionEffect &effect, bool mine)
{
    audio.play(Audio::Sfx::ShieldBreak);
    if (mine)
        react(Mood::Surprised);
    run.shakeAtLeast(0.3f);
    particles.emit(centerOf(effect), 36, SHIELD_COLOR, 8, 0.6f, 10);
    particles.emit(centerOf(effect), 12, sf::Color::White, 5, 0.4f, 10);
}

void GameScreen::powerUpTaken(const CollisionEffect &effect, bool mine)
{
    static const std::map<char, Audio::Sfx> sounds = {
        {SHIELD_TILE, Audio::Sfx::Shield}, {SANDEVISTAN_TILE, Audio::Sfx::Sandevistan},
        {BERSERK_TILE, Audio::Sfx::Berserk}};

    if (mine) {
        audio.play(sounds.at(effect.type));
        react(Mood::Pickup);
    }
    particles.emit(centerOf(effect), 24, powerUpColor(effect.type), 6, 0.5f, 0);
}

// A row of eddies taken at once counts up on one "+N", where the last one was
void GameScreen::eddyTaken(const CollisionEffect &effect)
{
    if (coin_clock.getElapsedTime().asSeconds() > EFFECT_DURATION)
        coin_streak = 0;
    coin_streak++;
    coin_clock.restart();
    last_coin = centerOf(effect);
    audio.play(Audio::Sfx::Coin);
    particles.emit(last_coin, 6, NEON_PINK, 5, 0.45f, 0);
    particles.emit(last_coin, 2, NEON_YELLOW, 3, 0.35f, 0);
}

// The cut of Mikasa's blade across a drone: a white slash that stretches out, then thins and fades
void GameScreen::drawCuts()
{
    float tile_size = run.view.tile_size;

    std::erase_if(cuts, [](const Cut &cut) { return cut.age.getElapsedTime().asSeconds() > CUT_TIME; });
    for (const Cut &cut : cuts) {
        float t = cut.age.getElapsedTime().asSeconds() / CUT_TIME;
        sf::RectangleShape slash({tile_size * (1.0f + 1.2f * std::min(1.0f, t * 4)), tile_size * 0.12f * (1 - t)});
        slash.setOrigin(slash.getSize() / 2.f);
        slash.setPosition(run.toScreen(cut.pos));
        slash.setRotation(sf::degrees(-30));
        slash.setFillColor(withAlpha(sf::Color::White, static_cast<uint8_t>(255 * (1 - t))));
        canvas.draw(slash, sf::BlendAdd);
        canvas.halo(run.toScreen(cut.pos), tile_size * 0.6f * (1 - t), withAlpha(NEON_CYAN, static_cast<uint8_t>(120 * (1 - t))));
    }
}

// A jagged bolt, redrawn every frame so it crackles: a wide purple glow under a thin white core
void GameScreen::drawZaps()
{
    static constexpr int SEGMENTS = 6;
    float tile_size = run.view.tile_size;

    std::erase_if(zaps, [](const Zap &zap) { return zap.age.getElapsedTime().asSeconds() > ZAP_TIME; });
    for (const Zap &zap : zaps) {
        float fade = 1 - zap.age.getElapsedTime().asSeconds() / ZAP_TIME;
        sf::Vector2f along = zap.to - zap.from;
        sf::Vector2f across = sf::Vector2f(-along.y, along.x) / std::max(0.01f, along.length());
        sf::Vector2f last = zap.from;

        for (int i = 1; i <= SEGMENTS; i++) {
            sf::Vector2f next = zap.from + along * (static_cast<float>(i) / SEGMENTS);
            if (i < SEGMENTS)
                next += across * randf(-0.3f, 0.3f);
            sf::Vector2f a = run.toScreen(last);
            sf::Vector2f b = run.toScreen(next);
            for (auto [width, color] : {std::pair{0.18f, NEON_PURPLE}, {0.06f, sf::Color::White}}) {
                sf::RectangleShape segment({(b - a).length(), tile_size * width * fade});
                segment.setOrigin({0, segment.getSize().y / 2});
                segment.setPosition(a);
                segment.setRotation((b - a).angle());
                segment.setFillColor(withAlpha(color, static_cast<uint8_t>(255 * fade)));
                canvas.draw(segment, sf::BlendAdd);
            }
            last = next;
        }
    }
}

void GameScreen::drawEffects()
{
    std::erase_if(effects, [](const ActiveEffect &e) {
        return e.clock.getElapsedTime().asSeconds() > EFFECT_DURATION;
    });

    // Your eddies: one "+N" floating up, not a text per coin (dozens of them made phones stutter)
    float since_coin = coin_clock.getElapsedTime().asSeconds() / EFFECT_DURATION;
    if (coin_streak > 0 && since_coin < 1)
        canvas.text(std::format("+{}", coin_streak), 34, withAlpha(NEON_CYAN, static_cast<uint8_t>(255 * (1 - since_coin))),
                    run.toScreen(last_coin + sf::Vector2f(0, -0.5f - since_coin)));

    for (const ActiveEffect &active : effects) {
        float t = active.clock.getElapsedTime().asSeconds() / EFFECT_DURATION;
        auto alpha = static_cast<uint8_t>(255 * (1 - t));
        sf::Vector2f tile(active.effect.x, active.effect.y);

        if (active.effect.type == COIN_TILE) {
            // The eddy is sucked into whoever took it (from afar with the magnet), swelling and fading out
            auto taker = run.display_positions.find(active.effect.player);
            if (taker != run.display_positions.end()) {
                float fly = std::min(1.0f, t / COIN_FLY_SHARE);
                tile += (taker->second + sf::Vector2f(0, 0.5f) - tile) * fly * fly;
            }
            tiles.drawSprite(sprites.coin, tile, 0.55f + t, withAlpha(sf::Color::White, alpha));
        } else if (active.effect.type == DRONE_TILE || active.effect.type == TITAN_KILL) {
            // Blast ring, and the bounty for whoever took it down
            bool titan = active.effect.type == TITAN_KILL;
            sf::Vector2f center = run.toScreen(tile + sf::Vector2f(0.5f, titan ? -3 : 0.5f));
            canvas.halo(center, run.view.tile_size * (0.3f + t) * (titan ? 3 : 1), withAlpha(NEON_ORANGE, alpha / 2));
            if (active.mine)
                canvas.text(std::format("+{}", active.bounty), titan ? 48 : 34,
                            withAlpha(NEON_YELLOW, alpha), center - sf::Vector2f(0, t * run.view.tile_size));
        } else if (active.effect.type == SPEAR_BLAST) {
            // Lightning from floor to ceiling, a column as wide as the blast
            float width = run.view.tile_size * (2 * SPEAR_BLAST_RADIUS + 1) * (1 - t * 0.5f);
            sf::Vector2f top = run.toScreen({active.effect.x + 0.5f, 0});
            float height = run.toScreen({0, run.floor()}).y - top.y;
            for (auto [scale, color] : {std::pair{1.0f, NEON_CYAN}, {0.35f, sf::Color::White}}) {
                sf::RectangleShape column({width * scale, height});
                column.setPosition({top.x - width * scale / 2, top.y});
                column.setFillColor(withAlpha(color, alpha));
                canvas.draw(column, sf::BlendAdd);
            }
        } else if (isPowerUp(active.effect.type)) {
            // What you just grabbed floats up
            if (active.mine)
                canvas.text(powerUpName(active.effect.type), 34, withAlpha(powerUpColor(active.effect.type), alpha),
                            run.toScreen(tile + sf::Vector2f(0.5f, -t)));
        } else {
            // A death flashes red, a broken shield green
            sf::RectangleShape flash({Screen::WIDTH, Screen::HEIGHT});
            flash.setFillColor(withAlpha(active.effect.type == SHIELD_BREAK ? SHIELD_COLOR : NEON_RED, alpha / 2));
            canvas.draw(flash);
        }
    }
}
