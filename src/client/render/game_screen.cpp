// The run on screen: phases, world and speed lines (hit effects: game_effects.cpp, HUD: game_hud.cpp)

#include "game_screen.hpp"
#include "backdrops.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include "../settings.hpp"
#include "../wallet.hpp"
#include "../../common/tuning.hpp"
#include <algorithm>
#include <format>

using namespace Palette;

GameScreen::GameScreen(Canvas &canvas, const Sprites &sprites, const Backdrops &backdrops, Audio &audio)
    : canvas(canvas), sprites(sprites), backdrops(backdrops), audio(audio),
      tiles(canvas, sprites, run), runners(canvas, sprites, run, audio, particles),
      titans(canvas, sprites, run, audio), snipers(canvas, run, audio), smasher(canvas, sprites, run, audio), shots(canvas, run, audio, particles),
      ultimates(canvas, sprites, run, audio, particles) {
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
        recordSoloBests(state);
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
    snipers.reset();
    smasher.reset();
    shots.clear();
    ultimates.clear();
    effects.clear();
    speed_lines.clear();
    shown_backdrop = -1;
    heard_countdown = -1;
    drones_down = 0;
    hit_stop = 0;
    cuts.clear();
    zaps.clear();
    bounced.clear();
    coin_streak = 0;
    last_drone.clear();
    new_record = false;
    new_drone_record = false;
    bests = Bests::load();
    ultimate_key = keyName(Settings::load().ultimate_key);
    ultimate_was_ready = false;
    spears_shown = 0;
    style_shown = 0;
    mood = Mood::Idle;
    kill_streak = 0;
    head = head_target = 0;
    head_until = 0;
    upgrades = Wallet::load().upgrades;
    sandevistan_time = sandevistanTime(upgrades);
    titan_time = titanTime(upgrades);
    trauma_ready = upgrades[UPGRADE_TRAUMA] > 0;
    death_cause = 0;
}

// Solo runs chase the personal bests: the distance in each mode, the drones down in CYBERPSYCHO. Not
// with a custom tuning: an easier game would beat the real records.
void GameScreen::recordSoloBests(GameState &state)
{
    auto players = state.getPlayers();
    auto mine = players.find(state.getMyPlayerNumber());

    if (players.size() != 1 || mine == players.end() || Tuning::custom())
        return;

    int distance = static_cast<int>(mine->second.x);
    cyberpsycho_run = state.getMode() == GameMode::Cyberpsycho;
    new_record = distance > bests.distance(cyberpsycho_run);
    new_drone_record = cyberpsycho_run && drones_down > bests.drones;
    if (new_record)
        bests.distance(cyberpsycho_run) = distance;
    if (new_drone_record)
        bests.drones = drones_down;
    if (new_record || new_drone_record)
        bests.save();
}

// The run's eddies go to the wallet, to buy runners with
void GameScreen::bankEddies(GameState &state)
{
    auto players = state.getPlayers();
    auto mine = players.find(state.getMyPlayerNumber());

    if (mine == players.end())
        return;
    Wallet::earn(mine->second.score);
    Wallet saved = Wallet::load();
    wallet = saved.eddies;
    next_goal = saved.nextGoal();
}

//=============================================================================
// World
//=============================================================================

void GameScreen::update(GameState &state, const std::map<int, PlayerState> &players, int me, float dt)
{
    run.roster = state.getCharacters();

    // A hit-stop freezes the world: the runners, the sparks and the wind hold still for a blink
    float world_dt = hit_stop > 0 ? 0 : dt;
    hit_stop -= dt;
    run.follow(players, me, world_dt);
    updateSpeedLines(world_dt);
    particles.update(world_dt);

    // Shots first: a blade kill comes on the same tick as the cut itself
    shots.handle(state);
    for (const CollisionEffect &effect : state.takeEffects())
        addEffect(effect, me);
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

    // The world view shakes on hits and blasts, the HUD doesn't. It eases out over its last 0.15 s.
    sf::View ui_view = canvas.screenView();
    sf::View world_view = ui_view;
    if (run.shake > 0) {
        float strength = run.shake_strength * std::min(1.0f, run.shake / 0.15f);
        world_view.move({randf(-strength, strength), randf(-strength, strength)});
        run.shake -= dt;
    }

    canvas.target().setView(world_view);
    drawDistrict(state.getDistrict());
    drawSpeedLines();
    std::set<std::pair<int, int>> destroyed = state.getDestroyedDrones();
    tiles.draw(state.getCollectedCoins(), destroyed, state.getPinataDrops());
    if (phase == Phase::Running && state.getMode() == GameMode::Cyberpsycho)
        drawHackTarget(players, me, destroyed);
    std::vector<TitanState> titan_states = state.getTitans();
    titans.draw(titan_states);
    titan_coming = std::ranges::any_of(titan_states, [&](const TitanState &titan) {
        return run.toScreen({titan.x, 0}).x > Screen::WIDTH;
    });
    run.cyberpsycho = state.getMode() == GameMode::Cyberpsycho;
    runners.draw(players, me, phase == Phase::Running);
    drawTraumaTeam();
    ultimates.draw();
    shots.draw();
    drawCuts();
    drawZaps();
    particles.draw(canvas, run.view);

    canvas.target().setView(ui_view);
    drawFlashAndCallout();
    drawEffects();
    titans.drawWarnings(titan_states);
    snipers.draw(state.getSnipers());
    smasher.draw(state.getBoss(), state.getMode() == GameMode::Cyberpsycho);
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

// Streaks start past 7 tiles/s and thicken up to top speed: about 20 of them there, more washed the
// screen out into a blur
void GameScreen::updateSpeedLines(float dt)
{
    size_t wanted = std::clamp(static_cast<int>((run.run_speed - 7) * 2), 0, 30);

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
        line.setFillColor(withAlpha(sf::Color::White, 45));
        canvas.draw(line, sf::BlendAdd);

        line.setSize({streak.length * 0.25f, 3});
        line.setFillColor(withAlpha(NEON_CYAN, 140));
        canvas.draw(line, sf::BlendAdd);
    }
}
