// The run on screen: phases, world, speed lines and hit effects

#include "game_screen.hpp"
#include "backdrops.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include "../settings.hpp"
#include "../wallet.hpp"
#include <algorithm>
#include <format>

using namespace Palette;

GameScreen::GameScreen(Canvas &canvas, const Sprites &sprites, const Backdrops &backdrops, Audio &audio)
    : canvas(canvas), sprites(sprites), backdrops(backdrops), audio(audio),
      tiles(canvas, sprites, run), runners(canvas, sprites, run, audio, particles),
      titans(canvas, sprites, run, audio), shots(canvas, run, audio, particles),
      ultimates(canvas, run, audio, particles) {
}

void GameScreen::draw(GameState &state, Phase phase, bool connected, float dt)
{
    for (const auto &data : state.takeMapData())
        run.map.applyData(data);

    if (phase != last_phase)
        onPhaseChange(state, phase);

    if (phase != Phase::Waiting && run.map.getHeight() > 0)
        drawWorld(state, state.getPlayers(), state.getMyPlayerNumber(), dt);

    if (!connected && phase != Phase::Over)
        drawConnectionLost();
}

//=============================================================================
// Phases
//=============================================================================

void GameScreen::onPhaseChange(GameState &state, Phase phase)
{
    Phase previous = last_phase;

    last_phase = phase;
    phase_clock.restart();

    // A run starts with its countdown, or straight away if the countdown went unseen
    if (phase == Phase::Countdown || (phase == Phase::Running && previous != Phase::Countdown))
        resetRun();
    if (phase == Phase::Running && previous == Phase::Countdown)
        audio.play(Audio::Sfx::Go);
    if (phase == Phase::Over) {
        recordSoloDistance(state);
        bankEddies(state);
        audio.play(Audio::Sfx::GameOver);
    }
}

// New run (first one or rematch): forget the previous one
void GameScreen::resetRun()
{
    run.reset();
    particles.clear();
    titans.reset();
    shots.clear();
    ultimates.clear();
    effects.clear();
    speed_lines.clear();
    shown_backdrop = -1;
    heard_countdown = -1;
    drones_down = 0;
    new_record = false;
    best_distance = loadBestDistance();
    ultimate_key = keyName(Settings::load().ultimate_key);
    ultimate_was_ready = false;
    sandevistan_time = sandevistanTime(Wallet::load().upgrades);
}

// Solo runs chase the personal best
void GameScreen::recordSoloDistance(GameState &state)
{
    auto players = state.getPlayers();
    auto mine = players.find(state.getMyPlayerNumber());

    if (players.size() != 1 || mine == players.end())
        return;

    int distance = static_cast<int>(mine->second.x);
    new_record = distance > best_distance;
    if (new_record) {
        best_distance = distance;
        saveBestDistance(distance);
    }
}

// The run's eddies go to the wallet, to buy runners with
void GameScreen::bankEddies(GameState &state)
{
    auto players = state.getPlayers();
    auto mine = players.find(state.getMyPlayerNumber());

    if (mine == players.end())
        return;
    Wallet::earn(mine->second.score);
    wallet = Wallet::load().eddies;
}

//=============================================================================
// World
//=============================================================================

void GameScreen::update(GameState &state, const std::map<int, PlayerState> &players, int me, float dt)
{
    run.roster = state.getCharacters();
    run.follow(players, me, dt);
    updateSpeedLines(dt);
    particles.update(dt);

    for (const CollisionEffect &effect : state.takeEffects())
        addEffect(effect, me);
    shots.handle(state);
    ultimates.handle(state);
}

// Countdown beeps, and the jetpack rumble while your thrust is on
void GameScreen::updateSounds(GameState &state, Phase phase, const std::map<int, PlayerState> &players, int me)
{
    int countdown = state.getCountdown();
    auto mine = players.find(me);
    bool alive = !run.death_clocks.count(me);

    if (phase == Phase::Countdown && countdown > 0 && countdown != heard_countdown) {
        heard_countdown = countdown;
        audio.play(Audio::Sfx::Beep);
    }

    bool flying = mine != players.end() && mine->second.form != Form::Shifting && mine->second.form != Form::Titan;
    audio.setJetpack(phase == Phase::Running && alive && flying && mine->second.jet_active);
}

void GameScreen::drawWorld(GameState &state, const std::map<int, PlayerState> &players, int me, float dt)
{
    Phase phase = last_phase;
    update(state, players, me, dt);
    updateSounds(state, phase, players, me);

    // World view shakes when someone gets zapped, the HUD doesn't
    sf::View ui_view = canvas.screenView();
    sf::View world_view = ui_view;
    if (run.shake > 0) {
        world_view.move({randf(-12, 12), randf(-12, 12)});
        run.shake -= dt;
    }

    canvas.target().setView(world_view);
    drawDistrict(state.getDistrict());
    drawSpeedLines();
    std::set<std::pair<int, int>> destroyed = state.getDestroyedDrones();
    tiles.draw(state.getCollectedCoins(), destroyed);
    if (phase == Phase::Running && state.getMode() == GameMode::Cyberpsycho)
        drawHackTarget(players, me, destroyed);
    std::vector<TitanState> titan_states = state.getTitans();
    titans.draw(titan_states);
    runners.draw(players, me, phase == Phase::Running);
    ultimates.draw();
    shots.draw();
    particles.draw(canvas, run.view);

    canvas.target().setView(ui_view);
    drawFlashAndCallout();
    drawEffects();
    titans.drawWarnings(titan_states);
    if (players.count(me))
        drawPowerUps(players.at(me), me);
    drawHUD(players, me, state.getMode() == GameMode::Cyberpsycho);
    drawCountdown(phase, state.getCountdown());
    if (phase != Phase::Over)
        drawBanner();

    // Leave time to watch the zap before the curtain falls
    if (phase == Phase::Over && phase_clock.getElapsedTime().asSeconds() > GAME_OVER_DELAY)
        drawGameOver(state, players, me);
}

// Playing Lucy with RAM left: a pulsing ring on what F would hack, worked out like the server does
void GameScreen::drawHackTarget(const std::map<int, PlayerState> &players, int me,
                                const std::set<std::pair<int, int>> &destroyed)
{
    auto mine = players.find(me);
    auto pos = run.display_positions.find(me);

    if (mine == players.end() || pos == run.display_positions.end() || mine->second.ram == 0 ||
        mine->second.form != Form::Normal || run.death_clocks.count(me) ||
        weaponOf(run.characterOf(me)) != Weapon::Hack)
        return;

    auto [x, y] = hackTarget(run.map, pos->second.x, pos->second.y, Run::PLAYER_HEIGHT, [&](int x, int y) {
        return destroyed.count({x, y}) || run.hackLeft(x, y) > 0 || run.burnt_zappers.count({x, y});
    });
    if (x >= 0)
        tiles.drawSprite(sprites.hack_target, {static_cast<float>(x), static_cast<float>(y)},
                         1.4f + 0.12f * std::sin(canvas.time() * 10), sf::Color::White);
}

// "ENTERING <district>" each time the scenery changes
void GameScreen::drawDistrict(int district)
{
    int current = backdrops.drawDistrict(canvas, run.view, district);

    if (current != shown_backdrop) {
        shown_backdrop = current;
        banner = "ENTERING " + backdrops[current].name;
        banner_tagline = backdrops[current].tagline;
        banner_clock.restart();
    }
}

//=============================================================================
// Speed Lines
//=============================================================================

// Streaks start past 7 tiles/s and thicken up to top speed
void GameScreen::updateSpeedLines(float dt)
{
    size_t wanted = std::clamp(static_cast<int>((run.run_speed - 7) * 4), 0, 60);

    while (speed_lines.size() < wanted) {
        float speed = run.run_speed * run.view.tile_size * randf(1.5f, 2.5f);
        sf::Vector2f pos(Screen::WIDTH + randf(0, Screen::WIDTH), randf(Screen::PLAY_TOP, Screen::PLAY_BOTTOM));
        speed_lines.push_back({pos, randf(80, 240), speed});
    }

    for (SpeedLine &line : speed_lines)
        line.pos.x -= line.speed * dt;
    std::erase_if(speed_lines, [](const SpeedLine &line) { return line.pos.x + line.length < 0; });
    if (speed_lines.size() > wanted)
        speed_lines.resize(wanted);
}

void GameScreen::drawSpeedLines()
{
    sf::RectangleShape line;

    // Brighter at the head, so each streak reads as moving left
    for (const SpeedLine &streak : speed_lines) {
        line.setSize({streak.length, 3});
        line.setPosition(streak.pos);
        line.setFillColor(withAlpha(sf::Color::White, 70));
        canvas.draw(line, sf::BlendAdd);

        line.setSize({streak.length * 0.25f, 3});
        line.setFillColor(withAlpha(NEON_CYAN, 140));
        canvas.draw(line, sf::BlendAdd);
    }
}

//=============================================================================
// Effects
//=============================================================================

void GameScreen::addEffect(const CollisionEffect &effect, int me)
{
    sf::Vector2f tile_center(effect.x + 0.5f, effect.y + 0.5f);

    if (effect.type == ZAPPER_TILE) {
        audio.play(Audio::Sfx::Zap);
        run.shake = 0.5f;
        run.death_clocks.try_emplace(effect.player);
        particles.emit(tile_center, 40, NEON_CYAN, 9, 0.7f, 12);
        particles.emit(tile_center, 20, sf::Color::White, 6, 0.5f, 12);
    } else if (effect.type == TITAN_HIT) {
        audio.play(Audio::Sfx::Stomp);
        run.shake = 0.6f;
        run.death_clocks.try_emplace(effect.player);
        particles.emit(tile_center, 40, NEON_RED, 9, 0.7f, 12);
        particles.emit(tile_center, 20, sf::Color::White, 6, 0.5f, 12);
    } else if (effect.type == TITAN_KILL) {
        // Steam bursting from the nape
        audio.play(Audio::Sfx::Explosion);
        run.shakeAtLeast(0.3f);
        particles.emit(tile_center + sf::Vector2f(0, -3), 50, sf::Color(230, 230, 230), 7, 1.0f, -4);
        particles.emit(tile_center + sf::Vector2f(0, -3), 20, NEON_RED, 6, 0.6f, 8);
    } else if (effect.type == DRONE_TILE) {
        audio.play(Audio::Sfx::Explosion);
        run.shakeAtLeast(0.15f);
        drones_down += effect.player == me;
        shots.land(effect.player, tile_center.x);
        particles.emit(tile_center, 30, NEON_ORANGE, 8, 0.6f, 6);
        particles.emit(tile_center, 15, NEON_RED, 5, 0.5f, 6);
    } else if (effect.type == SPEAR_BLAST) {
        audio.play(Audio::Sfx::Thunder);
        run.shakeAtLeast(0.35f);
        shots.land(effect.player, tile_center.x);
        run.burnZappers(effect.x - SPEAR_BLAST_RADIUS, effect.x + SPEAR_BLAST_RADIUS);
        for (int i = 0; i < 4; i++)
            particles.emit({effect.x + 0.5f, randf(0, run.floor())}, 10, i % 2 ? NEON_CYAN : sf::Color::White, 6, 0.5f, 0);
    } else if (effect.type == ZAPPER_HACK) {
        // The whole zapper goes off: sparks on its nodes, nothing drawn over the screen (the shot plays the sound)
        for (std::pair<int, int> node : zapperNodes(run.map, effect.x, effect.y)) {
            run.hacked_zappers[node] = {sf::Clock(), HACK_TIME};
            particles.emit({node.first + 0.5f, node.second + 0.5f}, 10, NEON_PURPLE, 4, 0.4f, 0);
        }
        return;
    } else if (isPowerUp(effect.type)) {
        static const std::map<char, Audio::Sfx> sounds = {
            {SHIELD_TILE, Audio::Sfx::Shield}, {SANDEVISTAN_TILE, Audio::Sfx::Sandevistan},
            {BERSERK_TILE, Audio::Sfx::Berserk}};
        if (effect.player == me)
            audio.play(sounds.at(effect.type));
        particles.emit(tile_center, 24, powerUpColor(effect.type), 6, 0.5f, 0);
    } else if (effect.type == SHIELD_BREAK) {
        audio.play(Audio::Sfx::ShieldBreak);
        run.shakeAtLeast(0.3f);
        particles.emit(tile_center, 36, SHIELD_COLOR, 8, 0.6f, 10);
        particles.emit(tile_center, 12, sf::Color::White, 5, 0.4f, 10);
    } else if (effect.player == me) {
        audio.play(Audio::Sfx::Coin);
        particles.emit(tile_center, 14, NEON_PINK, 5, 0.45f, 0);
        particles.emit(tile_center, 6, NEON_YELLOW, 3, 0.35f, 0);
    } else {
        return;
    }
    effects.push_back({effect, sf::Clock(), effect.player == me});
}

void GameScreen::drawEffects()
{
    std::erase_if(effects, [](const ActiveEffect &e) {
        return e.clock.getElapsedTime().asSeconds() > EFFECT_DURATION;
    });

    for (const ActiveEffect &active : effects) {
        float t = active.clock.getElapsedTime().asSeconds() / EFFECT_DURATION;
        auto alpha = static_cast<uint8_t>(255 * (1 - t));
        sf::Vector2f tile(active.effect.x, active.effect.y);

        if (active.effect.type == COIN_TILE) {
            // Coin pops: swells and fades out while +1 floats up
            tiles.drawSprite(sprites.coin, tile, 0.55f + t, withAlpha(sf::Color::White, alpha));
            canvas.text("+1", 30, withAlpha(NEON_CYAN, alpha), run.toScreen(tile + sf::Vector2f(0.5f, -t)));
        } else if (active.effect.type == DRONE_TILE || active.effect.type == TITAN_KILL) {
            // Blast ring, and the bounty for whoever took it down
            bool titan = active.effect.type == TITAN_KILL;
            sf::Vector2f center = run.toScreen(tile + sf::Vector2f(0.5f, titan ? -3 : 0.5f));
            canvas.halo(center, run.view.tile_size * (0.3f + t) * (titan ? 3 : 1), withAlpha(NEON_ORANGE, alpha / 2));
            if (active.mine)
                canvas.text(std::format("+{}", titan ? TITAN_BOUNTY : DRONE_BOUNTY), titan ? 48 : 34,
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
