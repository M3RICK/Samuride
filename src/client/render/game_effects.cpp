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

    // A drone's contact is reported as a zapper's: the map tells them apart
    if (mine && (effect.type == ZAPPER_TILE || effect.type == TITAN_HIT || effect.type == SNIPER_HIT || effect.type == BOSS_HIT))
        death_cause = effect.type == ZAPPER_TILE && run.map.getTile(effect.x, effect.y) == DRONE_TILE ? DRONE_TILE : effect.type;

    switch (effect.type) {
        case ZAPPER_TILE:
            playerDown(effect, Audio::Sfx::Zap, NEON_CYAN, 0.5f, 18);
            break;
        case TITAN_HIT:
            playerDown(effect, Audio::Sfx::Stomp, NEON_RED, 0.6f, 22);
            break;
        case SNIPER_HIT:
        case BOSS_HIT:
            playerDown(effect, Audio::Sfx::Explosion, NEON_ORANGE, 0.6f, 22);
            break;
        case BOSS_KILL:
            bossDown(effect, mine);
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
        case TRAUMA_RESCUE:
            traumaRescue(effect, mine);
            break;
        case TRAUMA_OFFER:
            // The hit that should have killed you: the run waits on your answer (drawTraumaOffer)
            run.shakeAtLeast(0.4f);
            if (mine) {
                audio.play(Audio::Sfx::ShieldBreak);
                react(Mood::Surprised);
                trauma_offer_clock.restart();
            }
            return;
        case JACKPOT:
            jackpot(effect, mine);
            break;
        case PINATA_DROP:
            pinataBurst(effect);
            return;
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
    int bounty = effect.type == TITAN_KILL ? TITAN_BOUNTY : effect.type == BOSS_KILL ? BOSS_BOUNTY
                 : static_cast<int>(std::lround(DRONE_BOUNTY * STYLE_BOUNTY[style_shown]));
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

// Adam Smasher blowing up where he stood
void GameScreen::bossDown(const CollisionEffect &effect, bool mine)
{
    sf::Vector2f chest = centerOf(effect) + sf::Vector2f(1, 1);

    if (mine)
        react(Mood::Frenzy);
    audio.play(Audio::Sfx::Nuke);
    run.shakeAtLeast(1.0f, 24);
    particles.emit(chest, 80, NEON_ORANGE, 10, 1.2f, 4);
    particles.emit(chest, 40, NEON_RED, 8, 1.0f, 8);
    particles.emit(chest, 30, sf::Color::White, 6, 0.6f, 0);
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

    // Taken again while it runs: the server starts its timer over, so does the gauge
    PlayerAnim &anim = run.player_anims[effect.player];
    if (effect.type == SANDEVISTAN_TILE)
        anim.sprint_clock.restart();
    if (effect.type == BERSERK_TILE)
        anim.berserk_clock.restart();
    if (mine) {
        audio.play(sounds.at(effect.type));
        react(Mood::Pickup);
    }
    particles.emit(centerOf(effect), 24, powerUpColor(effect.type), 6, 0.5f, 0);
}

// The Trauma Team is called: sirens, and the runner down while it comes (drawTraumaTeam), then the bill
void GameScreen::traumaRescue(const CollisionEffect &effect, bool mine)
{
    audio.play(Audio::Sfx::Siren);
    run.shakeAtLeast(0.4f);
    run.rescues[effect.player].restart();
    if (!mine)
        return;
    react(Mood::Surprised);
    trauma_ready = false;
    run.announce({"TRAUMA TEAM", NEON_CYAN});
}

// The rescue's steps, in shares of it: the AV flies in and hovers, its medic slides down the cable,
// kneels over the runner (back up at Run::RESCUE_UP), climbs back, and the AV leaves
static constexpr float AV_IN = 0.08f, AV_HOVER = 0.3f, MEDIC_DOWN = 0.42f, MEDIC_UP = 0.68f, MEDIC_IN = 0.8f,
                       AV_OUT = 0.84f;
static constexpr float AV_HEIGHT = 2.4f;        // tiles, the AV's body
static constexpr float AV_BELLY = 5.5f;         // tiles over the floor while it hovers
static constexpr float MEDIC_HEIGHT = 1.8f;
static const sf::Vector2f AV_PATH{-14, -10};    // where it comes from and goes back to, from its hover

static float stepOf(float share, float from, float to)
{
    return std::clamp((share - from) / (to - from), 0.0f, 1.0f);
}

// Frame `share` of the way through an animation
static int frameOf(const Animation &anim, float share)
{
    return std::min(static_cast<int>(share * anim.frames), anim.frames - 1);
}

void GameScreen::drawLook(const Character &look, const Animation &anim, int frame, sf::Vector2f feet, float height)
{
    if (look.sheet.getSize().x == 0 || frame < 0)
        return;
    float scale = run.view.tile_size * height / look.body;
    sf::Sprite sprite(look.sheet);
    sprite.setTextureRect(look.frameRect(anim, frame));
    sprite.setOrigin({look.frame_size.x / 2.f, look.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(run.toScreen(feet));
    canvas.draw(sprite);
}

// The AV over each runner it picks up, and its medic on the cable or kneeling over them
void GameScreen::drawTraumaTeam()
{
    const Character &av = sprites.trauma_av;
    const Character &medic = sprites.trauma_medic;

    for (const auto &[player_num, since] : run.rescues) {
        float share = run.rescueShare(player_num);
        if (share < AV_IN)
            continue;
        float floor = run.floor();
        float medic_x = run.display_positions[player_num].x - 1.3f;
        sf::Vector2f hover{medic_x, floor - AV_BELLY};

        // The medic: down the cable, kneeling with the injector, back up
        if (share >= AV_HOVER && share < MEDIC_IN) {
            float drop = share < MEDIC_DOWN ? stepOf(share, AV_HOVER, MEDIC_DOWN) : 1 - stepOf(share, MEDIC_UP, MEDIC_IN);
            float feet = hover.y + MEDIC_HEIGHT + (floor - hover.y - MEDIC_HEIGHT) * drop;
            sf::Vector2f top = run.toScreen({medic_x, hover.y - 0.4f});
            sf::RectangleShape cable({3, run.toScreen({medic_x, feet - MEDIC_HEIGHT}).y - top.y});
            cable.setPosition(top - sf::Vector2f(1.5f, 0));
            cable.setFillColor(sf::Color(150, 160, 180));
            canvas.draw(cable);
            bool kneeling = share >= MEDIC_DOWN && share < MEDIC_UP;
            drawLook(medic, kneeling ? medic.revive : medic.run,
                     kneeling ? frameOf(medic.revive, stepOf(share, MEDIC_DOWN, Run::RESCUE_UP)) : 0,
                     {medic_x, feet}, MEDIC_HEIGHT);
            // The injector's flash as the runner gets up
            float flash = 1 - std::abs(share - Run::RESCUE_UP) / 0.05f;
            if (flash > 0)
                canvas.halo(run.toScreen({medic_x + 1.2f, floor - 0.5f}), run.view.tile_size * 2.5f * flash,
                            withAlpha(NEON_CYAN, static_cast<uint8_t>(200 * flash)));
        }

        // The AV, drawn over the cable's end
        if (share < AV_HOVER) {
            float step = stepOf(share, AV_IN, AV_HOVER);
            drawLook(av, av.arrive, frameOf(av.arrive, step), hover + AV_PATH * (1 - step) * (1 - step), AV_HEIGHT);
        } else if (share < AV_OUT) {
            drawLook(av, av.run, av.run.frameAt(canvas.time()), hover, AV_HEIGHT);
        } else {
            float step = stepOf(share, AV_OUT, 1);
            drawLook(av, av.leave, frameOf(av.leave, step), hover + AV_PATH * step * step, AV_HEIGHT);
        }
    }
}

void GameScreen::jackpot(const CollisionEffect &effect, bool mine)
{
    if (!mine)
        return;
    audio.play(Audio::Sfx::Jackpot);
    particles.emit(centerOf(effect), 30, NEON_YELLOW, 7, 0.7f, 6);
    particles.emit(centerOf(effect), 12, sf::Color::White, 5, 0.5f, 6);
}

// Confetti in every colour: the drone burst open
void GameScreen::pinataBurst(const CollisionEffect &effect)
{
    audio.play(Audio::Sfx::Pinata);
    for (sf::Color color : {NEON_PINK, NEON_YELLOW, NEON_CYAN, NEON_GREEN, NEON_PURPLE})
        particles.emit(centerOf(effect), 10, color, 7, 0.8f, 8);
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
        } else if (active.effect.type == DRONE_TILE || active.effect.type == TITAN_KILL || active.effect.type == BOSS_KILL) {
            // Blast ring, and the bounty for whoever took it down
            bool titan = active.effect.type != DRONE_TILE;
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
        } else if (active.effect.type == JACKPOT || active.effect.type == TRAUMA_RESCUE) {
            // The jackpot pays out over the eddy; the Trauma Team's bill floats up over the runner
            bool jackpot = active.effect.type == JACKPOT;
            if (active.mine)
                canvas.text(jackpot ? std::format("JACKPOT +{}", JACKPOT_EDDIES)
                                    : std::format("BILLED {}% OF THE RUN", static_cast<int>(TRAUMA_BILL * 100)),
                            jackpot ? 40 : 34, withAlpha(jackpot ? NEON_YELLOW : NEON_CYAN, alpha),
                            run.toScreen(tile + sf::Vector2f(0.5f, -1 - t)));
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
