/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include <algorithm>
#include <cmath>
#include <format>

//=============================================================================
// Game Screens
//=============================================================================

void Renderer::render(GameState &state, bool connected)
{
    float dt = frame_clock.restart().asSeconds();

    for (const auto &data : state.takeMapData())
        world_map.applyData(data);

    Phase phase = state.getPhase();
    if (phase != last_phase)
        onPhaseChange(state, phase);

    beginFrame(phase != Phase::Waiting);

    if (phase == Phase::Waiting)
        renderWaitingScreen();
    else
        renderGameScreen(state, phase, dt);

    if (!connected && phase != Phase::Over)
        renderConnectionLost();

    endFrame();
}

void Renderer::renderGameScreen(GameState &state, Phase phase, float dt)
{
    if (world_map.getHeight() == 0)
        return;

    auto players = state.getPlayers();
    int me = state.getMyPlayerNumber();
    updateWorld(state, players, me, dt);
    updateSounds(state, phase, players, me);

    // World view shakes when someone gets zapped, the HUD doesn't
    sf::View ui_view = screenView();
    sf::View world_view = ui_view;
    if (shake > 0) {
        world_view.move({randf(-12, 12), randf(-12, 12)});
        shake -= dt;
    }

    window.setView(world_view);
    renderDistrict(state.getDistrict());
    std::set<std::pair<int, int>> destroyed = state.getDestroyedDrones();
    renderMapTiles(state.getCollectedCoins(), destroyed);
    renderPlayers(players, me);
    renderShots(destroyed);
    renderParticles();

    window.setView(ui_view);
    renderEffects();
    renderHUD(players, me, state.getMode() == GameMode::Cyberpsycho);
    renderCountdown(phase, state.getCountdown());
    if (phase != Phase::Over)
        renderBanner();

    // Leave time to watch the zap before the curtain falls
    if (phase == Phase::Over && phase_clock.getElapsedTime().asSeconds() > GAME_OVER_DELAY)
        renderGameOver(state, players, me);
}

void Renderer::updateWorld(GameState &state, const std::map<int, PlayerState> &players, int me, float dt)
{
    tile_size = (PLAY_BOTTOM - PLAY_TOP) / world_map.getHeight();
    roster = state.getCharacters();

    smoothPositions(players, dt);
    updateCamera(me);
    updateParticles(dt);

    for (const CollisionEffect &effect : state.takeEffects())
        addEffect(effect, me);
    handleShots(state);
}

// Countdown beeps, and the jetpack rumble while your thrust is on
void Renderer::updateSounds(GameState &state, Phase phase, const std::map<int, PlayerState> &players, int me)
{
    int countdown = state.getCountdown();
    auto mine = players.find(me);
    bool alive = !death_clocks.count(me);

    if (phase == Phase::Countdown && countdown > 0 && countdown != heard_countdown) {
        heard_countdown = countdown;
        audio.play(Audio::Sfx::Beep);
    }

    audio.setJetpack(phase == Phase::Running && alive && mine != players.end() && mine->second.jet_active);
}

// "ENTERING <district>", for a moment after the scenery changes
void Renderer::renderBanner()
{
    if (banner_clock.getElapsedTime().asSeconds() >= BANNER_DURATION)
        return;

    drawGlitchText(banner, 54, NEON_CYAN, {SCREEN_WIDTH / 2, 200});
    window.draw(makeText(banner_tagline, 28, DIM_WHITE, {SCREEN_WIDTH / 2, 260}));
}

void Renderer::renderCountdown(Phase phase, int countdown)
{
    if (phase == Phase::Countdown && countdown > 0)
        drawGlitchText(std::to_string(countdown), 220, NEON_YELLOW, SCREEN_CENTER);
    else if (phase == Phase::Running && phase_clock.getElapsedTime().asSeconds() < 1.2f)
        drawGlitchText("BURN THE CITY", 120, NEON_RED, SCREEN_CENTER);
}

void Renderer::renderGameOver(GameState &state, const std::map<int, PlayerState> &players, int me)
{
    bool solo = players.size() == 1;
    auto mine = players.find(me);

    drawOverlay(190);

    if (solo && mine != players.end())
        renderSoloResults(mine->second);
    else
        renderVersusResults(players, state.getWinner(), me);

    if (phase_clock.getElapsedTime().asSeconds() >= PROMPT_DELAY)
        renderRematchPrompt(state, solo);
}

void Renderer::renderSoloResults(const PlayerState &me)
{
    sf::Vector2f center = SCREEN_CENTER;
    std::string result = std::format("{} M        {} EDDIES", static_cast<int>(me.x), me.score);

    drawGlitchText("FLATLINED", 140, NEON_RED, center + sf::Vector2f(0, -150));
    window.draw(makeText(result, 44, sf::Color::White, center + sf::Vector2f(0, 10)));

    if (new_record)
        drawGlitchText("NEW PERSONAL BEST", 48, NEON_YELLOW, center + sf::Vector2f(0, 95));
    else
        window.draw(makeText(std::format("BEST  {} M", best_distance), 32, DIM_WHITE, center + sf::Vector2f(0, 95)));
}

void Renderer::renderVersusResults(const std::map<int, PlayerState> &players, int winner, int me)
{
    sf::Vector2f center = SCREEN_CENTER;
    sf::Vector2f title_pos = center + sf::Vector2f(0, -150);
    auto mine = players.find(me);
    int distance = mine != players.end() ? static_cast<int>(mine->second.x) : 0;

    if (winner == me)
        drawGlitchText("LEGEND OF THE AFTERLIFE", 90, NEON_YELLOW, title_pos);
    else if (winner >= 0)
        drawGlitchText("FLATLINED", 140, NEON_RED, title_pos);
    else
        drawGlitchText("NO WINNERS IN NIGHT CITY", 90, NEON_CYAN, title_pos);

    std::string scores;
    for (const auto &[player_num, player] : players) {
        if (!scores.empty())
            scores += "        ";
        scores += playerLabel(player_num, me) + "  " + std::to_string(player.score) + " EDDIES";
    }

    window.draw(makeText(std::format("{} M", distance), 44, sf::Color::White, center + sf::Vector2f(0, 10)));
    window.draw(makeText(scores, 34, DIM_WHITE, center + sf::Vector2f(0, 80)));
}

void Renderer::renderRematchPrompt(GameState &state, bool solo)
{
    sf::Vector2f pos = SCREEN_CENTER + sf::Vector2f(0, 220);

    if (state.isRematchRequested())
        window.draw(makeText("WAITING FOR YOUR CHOOM...", 32, NEON_CYAN, pos));
    else
        window.draw(makeText(solo ? "[SPACE] RUN AGAIN     [ESC] MENU" : "[SPACE] REMATCH     [ESC] MENU",
                             32, NEON_YELLOW, pos));
}

void Renderer::renderConnectionLost()
{
    window.setView(screenView());
    drawOverlay(200);

    drawGlitchText("CONNECTION LOST", 110, NEON_RED, SCREEN_CENTER + sf::Vector2f(0, -40));
    window.draw(makeText("[ESC] BACK TO MENU", 32, NEON_YELLOW, SCREEN_CENTER + sf::Vector2f(0, 100)));
}

//=============================================================================
// Effects & Particles
//=============================================================================

void Renderer::addEffect(const CollisionEffect &effect, int me)
{
    sf::Vector2f tile_center(effect.x + 0.5f, effect.y + 0.5f);

    if (effect.type == 'e') {
        audio.play(Audio::Sfx::Zap);
        shake = 0.5f;
        death_clocks.try_emplace(effect.player);
        emitParticles(tile_center, 40, NEON_CYAN, 9, 0.7f, 12);
        emitParticles(tile_center, 20, sf::Color::White, 6, 0.5f, 12);
    } else if (effect.type == DRONE_TILE) {
        audio.play(Audio::Sfx::Explosion);
        shake = std::max(shake, 0.15f);
        drones_down += effect.player == me;
        emitParticles(tile_center, 30, NEON_ORANGE, 8, 0.6f, 6);
        emitParticles(tile_center, 15, NEON_RED, 5, 0.5f, 6);
    } else if (effect.player == me) {
        audio.play(Audio::Sfx::Coin);
        emitParticles(tile_center, 14, NEON_PINK, 5, 0.45f, 0);
        emitParticles(tile_center, 6, NEON_YELLOW, 3, 0.35f, 0);
    } else {
        return;
    }
    effects.push_back({effect, sf::Clock(), effect.player == me});
}

void Renderer::renderEffects()
{
    std::erase_if(effects, [](const ActiveEffect &e) {
        return e.clock.getElapsedTime().asSeconds() > EFFECT_DURATION;
    });

    for (const ActiveEffect &active : effects) {
        float t = active.clock.getElapsedTime().asSeconds() / EFFECT_DURATION;
        auto alpha = static_cast<uint8_t>(255 * (1 - t));
        sf::Vector2f tile(active.effect.x, active.effect.y);

        if (active.effect.type == 'c') {
            // Coin pops: swells and fades out while +1 floats up
            drawTileSprite(coin_texture, tile, 0.55f + t, withAlpha(sf::Color::White, alpha));
            window.draw(makeText("+1", 30, withAlpha(NEON_CYAN, alpha), toScreen(tile + sf::Vector2f(0.5f, -t))));
        } else if (active.effect.type == DRONE_TILE) {
            // Blast ring, and the bounty for whoever took it down
            sf::Vector2f center = toScreen(tile + sf::Vector2f(0.5f, 0.5f));
            drawHalo(center, tile_size * (0.3f + t), withAlpha(NEON_ORANGE, alpha / 2));
            if (active.mine)
                window.draw(makeText(std::format("+{}", DRONE_BOUNTY), 34, withAlpha(NEON_YELLOW, alpha),
                                     toScreen(tile + sf::Vector2f(0.5f, -t))));
        } else {
            sf::RectangleShape flash({SCREEN_WIDTH, SCREEN_HEIGHT});
            flash.setFillColor(withAlpha(NEON_RED, alpha / 2));
            window.draw(flash);
        }
    }
}

void Renderer::emitParticles(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
                             float gravity, sf::Vector2f base_vel)
{
    for (int i = 0; i < count; i++) {
        float angle = randf(0, 6.2832f);
        sf::Vector2f vel = base_vel + sf::Vector2f(std::cos(angle), std::sin(angle)) * randf(0.2f, 1) * speed;
        float particle_life = life * randf(0.6f, 1);

        particles.push_back({pos, vel, particle_life, particle_life, randf(4, 9), gravity, color});
    }
}

void Renderer::updateParticles(float dt)
{
    for (Particle &p : particles) {
        p.vel.y += p.gravity * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
    }
    std::erase_if(particles, [](const Particle &p) { return p.life <= 0; });
}

void Renderer::renderParticles()
{
    sf::RectangleShape shape;

    for (const Particle &p : particles) {
        shape.setSize({p.size, p.size});
        shape.setOrigin({p.size / 2, p.size / 2});
        shape.setPosition(toScreen(p.pos));
        shape.setFillColor(withAlpha(p.color, 255 * p.life / p.max_life));
        window.draw(shape, sf::BlendAdd);
    }
}

//=============================================================================
// HUD
//=============================================================================

void Renderer::renderHUD(const std::map<int, PlayerState> &players, int me, bool cyberpsycho)
{
    auto mine = display_positions.find(me);
    int distance = mine != display_positions.end() ? static_cast<int>(mine->second.x) : 0;
    float y = 90;

    window.draw(makeText(std::format("{:04} M", distance), 56, NEON_YELLOW, {24, 16}, {0, 0}));

    if (players.size() == 1 && best_distance > 0) {
        window.draw(makeText(std::format("BEST {} M", best_distance), 24, DIM_WHITE, {24, y}, {0, 0}));
        y += 36;
    }

    for (const auto &[player_num, player] : players) {
        renderHUDLine(player_num, player, me, y);
        y += 44;
    }

    if (cyberpsycho)
        window.draw(makeText(std::format("DRONES DOWN  {}", drones_down), 26, NEON_RED, {24, y}, {0, 0}));
}

// Portrait, then "YOU   12 EDDIES"
void Renderer::renderHUDLine(int player_num, const PlayerState &player, int me, float y)
{
    std::string line = playerLabel(player_num, me) + "   " + std::to_string(player.score) + " EDDIES";
    drawImage(characterOf(player_num).portrait, {24, y - 2}, 34, {0, 0}, sf::Color::White);
    window.draw(makeText(line, 28, player_num == me ? NEON_YELLOW : NEON_CYAN, {70, y}, {0, 0}));
}
