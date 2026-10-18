/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include "menu.hpp"
#include "state.hpp"
#include "../common/debug.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>

static float randf(float min, float max)
{
    return min + (max - min) * (std::rand() / static_cast<float>(RAND_MAX));
}

Renderer::Renderer()
    : shown_backdrop(-1), camera_x(0), tile_size(96), shake(0), best_distance(0), new_record(false),
      last_phase(Phase::Waiting), scanlines(sf::PrimitiveType::Lines) {
}

//=============================================================================
// Initialization
//=============================================================================

bool Renderer::initialize()
{
    createWindow();
    if (!window.isOpen())
        return false;

    loadAssets();
    loadAudio();

    for (float y = 0; y < SCREEN_HEIGHT; y += 4) {
        scanlines.append({{0, y}, sf::Color(0, 0, 0, 45)});
        scanlines.append({{SCREEN_WIDTH, y}, sf::Color(0, 0, 0, 45)});
    }
    return true;
}

// Everything is drawn in a fixed 1920x1080 space, the window just scales it to fit the screen
void Renderer::createWindow()
{
    sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    float fit = std::min({1.0f, desktop.x * 0.9f / SCREEN_WIDTH, desktop.y * 0.9f / SCREEN_HEIGHT});

    window.create(sf::VideoMode({static_cast<unsigned>(SCREEN_WIDTH * fit), static_cast<unsigned>(SCREEN_HEIGHT * fit)}),
                  "Samuride", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
}

void Renderer::loadAssets()
{
    // The corridor scrolls as one layer; the others split a far skyline from the walkway you run on
    backdrops.resize(3);
    backdrops[0].name = "THE CORRIDOR";
    backdrops[0].tagline = "Arasaka sublevel 3";
    backdrops[0].ceiling = 0.20f;
    backdrops[0].floor = 0.79f;
    backdrops[0].tint = sf::Color::White;
    backdrops[0].layers = {{0, 1, 0.4f, {}}};
    loadTexture(backdrops[0].texture, "assets/background/Resized.png");

    backdrops[1].name = "WORLDS COLLIDE";
    backdrops[1].tagline = "Night City, Zaun & the Walls";
    backdrops[1].ceiling = 0.05f;
    backdrops[1].floor = 0.825f;
    // Dimmed a little: its neons would swallow the coins and zappers
    backdrops[1].tint = sf::Color(200, 200, 215);
    loadTexture(backdrops[1].texture, "assets/background/worlds_collide.png");
    // Its far city chains into the Neon Citadel panorama (3 strips, sunset to night), cut above their walkways
    backdrops[1].extra_textures.resize(1);
    loadTexture(backdrops[1].extra_textures[0], "assets/background/neon_citadel.png");
    sf::Vector2u city = backdrops[1].texture.getSize();
    int citadel_width = backdrops[1].extra_textures[0].getSize().x;
    std::vector<Backdrop::Segment> citadel_strips = {
        {1, sf::IntRect({0, 0}, {citadel_width, 260})},
        {1, sf::IntRect({0, 283}, {citadel_width, 209})},
        {1, sf::IntRect({0, 513}, {citadel_width, 180})},
    };
    std::vector<Backdrop::Segment> journey = {{0, sf::IntRect({0, 0}, {static_cast<int>(city.x), static_cast<int>(city.y * 0.8f)})}};
    journey.insert(journey.end(), citadel_strips.begin(), citadel_strips.end());
    backdrops[1].layers = {{0, 0.8f, 0.12f, journey}, {0.8f, 1, 1, {}}};

    // Neon Citadel: the strips alone from the first metre, on the same walkway
    backdrops[2].name = "NEON CITADEL";
    backdrops[2].tagline = "From sunset to the Zaun night";
    backdrops[2].ceiling = backdrops[1].ceiling;
    backdrops[2].floor = backdrops[1].floor;
    backdrops[2].tint = backdrops[1].tint;
    loadTexture(backdrops[2].texture, "assets/background/worlds_collide.png");
    backdrops[2].extra_textures.resize(1);
    loadTexture(backdrops[2].extra_textures[0], "assets/background/neon_citadel.png");
    backdrops[2].layers = {{0, 0.8f, 0.12f, citadel_strips}, {0.8f, 1, 1, {}}};

    loadTexture(waiting_screen_texture, "assets/waiting_screen/Samuride.png");
    loadTexture(coin_texture, "assets/coins/coin.png");
    loadTexture(electric_texture, "assets/electric/zap.png");

    // Pixel art: textures stay unsmoothed so it stays crisp when scaled up
    characters.resize(CHARACTER_DIRS.size());
    for (size_t i = 0; i < CHARACTER_DIRS.size(); i++) {
        if (!characters[i].load("assets/" + CHARACTER_DIRS[i]))
            DEBUG_LOG("Failed to load character: " + CHARACTER_DIRS[i]);
    }

    if (!font.openFromFile("assets/font/jetpack_font.ttf") || !fallback_font.openFromFile("assets/font/Rajdhani-Bold.ttf"))
        DEBUG_LOG("Failed to load fonts, text will not be displayed");

    for (char32_t c = '!'; c <= '~'; c++) {
        if (font.getGlyph(c, 32, false).bounds.size.x == 0)
            missing_glyphs += static_cast<char>(c);
    }
}

void Renderer::loadTexture(sf::Texture &texture, const std::string &path)
{
    if (!texture.loadFromFile(path)) {
        DEBUG_LOG("Failed to load texture: " + path);
        return;
    }
    texture.setSmooth(true);
}

void Renderer::loadAudio()
{
    if (waiting_music.openFromFile("assets/music/waiting_screen.wav")) {
        waiting_music.setLooping(true);
        waiting_music.setVolume(50);
    }

    if (background_music.openFromFile("assets/music/ingame.wav")) {
        background_music.setLooping(true);
        background_music.setVolume(40);
    }
}

//=============================================================================
// Frame Lifecycle
//=============================================================================

void Renderer::beginFrame(bool in_game)
{
    sf::Music &wanted = in_game ? background_music : waiting_music;
    sf::Music &other = in_game ? waiting_music : background_music;

    if (wanted.getStatus() != sf::SoundSource::Status::Playing) {
        other.stop();
        wanted.play();
    }

    window.setView(sf::View(sf::FloatRect({0, 0}, {SCREEN_WIDTH, SCREEN_HEIGHT})));
    window.clear(sf::Color::Black);
}

void Renderer::endFrame()
{
    window.setView(sf::View(sf::FloatRect({0, 0}, {SCREEN_WIDTH, SCREEN_HEIGHT})));
    window.draw(scanlines);
    window.display();
}

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

void Renderer::onPhaseChange(GameState &state, Phase phase)
{
    last_phase = phase;
    phase_clock.restart();

    // New run (first one or rematch): forget the previous one
    if (phase == Phase::Countdown) {
        display_positions.clear();
        particles.clear();
        effects.clear();
        player_anims.clear();
        death_clocks.clear();
        shown_backdrop = -1;
        shake = 0;
        new_record = false;
        best_distance = loadBestDistance();
    }

    // Solo runs chase the personal best
    auto players = state.getPlayers();
    auto mine = players.find(state.getMyPlayerNumber());
    if (phase == Phase::Over && players.size() == 1 && mine != players.end()) {
        int distance = static_cast<int>(mine->second.x);
        new_record = distance > best_distance;
        if (new_record) {
            best_distance = distance;
            saveBestDistance(distance);
        }
    }
}

//=============================================================================
// Menu & Lobby
//=============================================================================

void Renderer::renderMenu(const MenuState &menu)
{
    frame_clock.restart();
    beginFrame(false);

    // The scenery keeps scrolling behind the menu: the corridor, or the district being picked
    float time = animation_clock.getElapsedTime().asSeconds();
    tile_size = (PLAY_BOTTOM - PLAY_TOP) / 10;
    camera_x = time * 3;

    if (menu.screen != MenuState::Screen::District) {
        renderBackdrop(backdrops[0], 1);
    } else if (menu.district == MenuLayout::DISTRICT_MIX) {
        renderBackdrop(backdrops[0], 1);
        renderBackdrop(backdrops[1], 0.5f + 0.5f * std::sin(time * 0.8f));
    } else {
        renderBackdrop(backdrops[menu.district], 1);
    }

    sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);

    if (menu.screen == MenuState::Screen::Character || menu.screen == MenuState::Screen::District) {
        if (menu.screen == MenuState::Screen::Character)
            renderCharacterSelect(menu);
        else
            renderDistrictSelect(menu);
        endFrame();
        return;
    }
    renderPoster();

    float x = MenuLayout::X;
    drawGlitchText("Wake the fuck up, Samurai.", 46, NEON_RED, {x, 220});
    window.draw(makeText("We have a city to burn.", 32, NEON_CYAN, {x, 290}));

    if (menu.screen == MenuState::Screen::Main) {
        for (int i = 0; i < MenuLayout::ITEM_COUNT; i++) {
            sf::Vector2f pos(x, MenuLayout::Y + i * MenuLayout::SPACING);

            if (i == menu.selected) {
                drawGlitchText(MenuLayout::ITEMS[i], 56, NEON_YELLOW, pos);

                float half_width = makeText(MenuLayout::ITEMS[i], 56, NEON_YELLOW, pos).getGlobalBounds().size.x / 2;
                sf::RectangleShape marker({10, 46});
                marker.setOrigin({5, 23});
                marker.setFillColor(NEON_RED);
                marker.setPosition(pos - sf::Vector2f(half_width + 30, 0));
                window.draw(marker);
                marker.setPosition(pos + sf::Vector2f(half_width + 30, 0));
                window.draw(marker);
            } else
                window.draw(makeText(MenuLayout::ITEMS[i], 46, DIM_WHITE, pos));
        }
    } else {
        window.draw(makeText("NETRUNNER ADDRESS", 40, NEON_CYAN, {x, MenuLayout::Y}));

        sf::RectangleShape field({MenuLayout::WIDTH, MenuLayout::HEIGHT});
        field.setOrigin(field.getSize() / 2.f);
        field.setPosition({x, MenuLayout::Y + 110});
        field.setFillColor(sf::Color(0, 0, 0, 180));
        field.setOutlineColor(NEON_CYAN);
        field.setOutlineThickness(3);
        window.draw(field);

        window.draw(makeText(menu.address + (blink() ? "_" : " "), 40, NEON_YELLOW, {x, MenuLayout::Y + 110}));
        window.draw(makeText("[ENTER] JACK IN     [ESC] BACK", 26, DIM_WHITE, {x, MenuLayout::Y + 220}));
    }

    if (!menu.error.empty())
        window.draw(makeText(menu.error, 30, NEON_RED, {x, MenuLayout::Y + 4 * MenuLayout::SPACING}));

    if (menu.best_distance > 0)
        window.draw(makeText(std::format("BEST RUN  {} M", menu.best_distance), 32, NEON_YELLOW, {x, 960}));
    window.draw(makeText("[ARROWS / MOUSE] SELECT     [ENTER] CONFIRM", 22, sf::Color(150, 150, 150), {x, 1030}));

    endFrame();
}

void Renderer::renderCharacterSelect(const MenuState &menu)
{
    int count = characters.size();
    float time = animation_clock.getElapsedTime().asSeconds();

    drawGlitchText("CHOOSE YOUR RUNNER", 80, NEON_YELLOW, {SCREEN_WIDTH / 2, 150});

    for (int i = 0; i < count; i++) {
        const Character &character = characters[i];
        bool selected = i == menu.character;
        sf::Vector2f size(MenuLayout::CARD_WIDTH, MenuLayout::CARD_HEIGHT);
        sf::Vector2f center(MenuLayout::cardX(i, count), MenuLayout::CARD_Y);
        float top = center.y - size.y / 2;
        sf::Color dim(255, 255, 255, 130);

        sf::RectangleShape card(size);
        card.setOrigin(size / 2.f);
        card.setPosition(center);
        card.setFillColor(sf::Color(10, 8, 20, 235));
        card.setOutlineColor(selected ? NEON_YELLOW : sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 90));
        card.setOutlineThickness(selected ? 5 : 2);
        window.draw(card);

        if (character.portrait.getSize().y > 0) {
            sf::Sprite portrait(character.portrait);
            float s = 150.f / character.portrait.getSize().y;
            portrait.setScale({s, s});
            portrait.setOrigin({character.portrait.getSize().x / 2.f, 0});
            portrait.setPosition({center.x, top + 25});
            if (!selected)
                portrait.setColor(dim);
            window.draw(portrait);
        }

        if (selected)
            drawGlitchText(character.name, 46, NEON_YELLOW, {center.x, top + 215});
        else
            window.draw(makeText(character.name, 40, DIM_WHITE, {center.x, top + 215}));

        // The selected runner shows off: running, then flying
        if (character.sheet.getSize().x > 0) {
            bool flying = selected && static_cast<int>(time / 2.5f) % 2 == 1;
            const Animation &anim = flying ? character.hover : character.run;
            // Cells are padded for trails and blades, they may spill a little out of the card
            float s = std::min(190.f / character.body, (size.x + 60) / character.frame_size.x);

            sf::Sprite sprite(character.sheet);
            sprite.setTextureRect(character.frameRect(anim, selected ? anim.frameAt(time) : 0));
            sprite.setOrigin({character.frame_size.x / 2.f, character.feet});
            sprite.setScale({s, s});
            sprite.setPosition({center.x, top + size.y - 30});
            if (!selected)
                sprite.setColor(dim);
            window.draw(sprite);
        }
    }

    window.draw(makeText("[LEFT / RIGHT] CHOOSE     [ENTER] CONFIRM     [ESC] BACK", 24,
                         sf::Color(150, 150, 150), {SCREEN_WIDTH / 2, 1030}));
}

void Renderer::renderDistrictSelect(const MenuState &menu)
{
    drawGlitchText("CHOOSE YOUR DISTRICT", 80, NEON_YELLOW, {SCREEN_WIDTH / 2, 150});

    for (int i = 0; i < MenuLayout::DISTRICT_COUNT; i++) {
        bool selected = i == menu.district;
        bool mix = i == MenuLayout::DISTRICT_MIX;
        sf::Vector2f size(MenuLayout::DISTRICT_WIDTH, MenuLayout::DISTRICT_HEIGHT);
        sf::Vector2f center(MenuLayout::districtX(i), MenuLayout::DISTRICT_Y);
        float top = center.y - size.y / 2;

        sf::RectangleShape card(size);
        card.setOrigin(size / 2.f);
        card.setPosition(center);
        card.setFillColor(sf::Color(10, 8, 20, 235));
        card.setOutlineColor(selected ? NEON_YELLOW : sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 90));
        card.setOutlineThickness(selected ? 5 : 2);
        window.draw(card);

        // Both worlds: half of each
        sf::FloatRect thumb({center.x - size.x / 2 + 20, top + 20}, {size.x - 40, 225});
        drawThumbnail(backdrops[mix ? 0 : i], thumb, 0, mix ? 0.5f : 1);
        if (mix)
            drawThumbnail(backdrops[1], thumb, 0.5f, 1);

        std::string name = mix ? "BOTH WORLDS" : backdrops[i].name;
        std::string tagline = mix ? "Back and forth, every 250 m" : backdrops[i].tagline;
        if (selected)
            drawGlitchText(name, 44, NEON_YELLOW, {center.x, top + 300});
        else
            window.draw(makeText(name, 40, DIM_WHITE, {center.x, top + 300}));
        window.draw(makeText(tagline, 26, selected ? NEON_CYAN : sf::Color(150, 150, 150), {center.x, top + 360}));
    }

    window.draw(makeText("[LEFT / RIGHT] CHOOSE     [ENTER] JACK IN     [ESC] BACK", 24,
                         sf::Color(150, 150, 150), {SCREEN_WIDTH / 2, 1030}));
}

void Renderer::renderPoster()
{
    if (waiting_screen_texture.getSize().y == 0)
        return;

    sf::Sprite poster(waiting_screen_texture);
    float scale = SCREEN_HEIGHT * 0.85f / waiting_screen_texture.getSize().y;

    poster.setScale({scale, scale});
    poster.setOrigin(sf::Vector2f(waiting_screen_texture.getSize()) / 2.f);
    poster.setPosition({SCREEN_WIDTH * 0.28f, SCREEN_HEIGHT / 2});
    window.draw(poster);
}

void Renderer::renderWaitingScreen()
{
    renderPoster();

    float text_x = MenuLayout::X;

    drawGlitchText("Wake the fuck up, Samurai.", 46, NEON_RED, {text_x, 220});
    window.draw(makeText("We have a city to burn.", 32, NEON_CYAN, {text_x, 290}));

    if (blink())
        window.draw(makeText("WAITING FOR ANOTHER CHOOM...", 34, NEON_YELLOW, {text_x, 520}));
    if (!lobby_info.empty())
        window.draw(makeText(lobby_info, 28, NEON_CYAN, {text_x, 600}));

    window.draw(makeText("[SPACE] JETPACK     [ESC] BACK TO MENU", 24, DIM_WHITE, {text_x, 1030}));
}

//=============================================================================
// Game Screens
//=============================================================================

void Renderer::renderGameScreen(GameState &state, Phase phase, float dt)
{
    if (world_map.getHeight() == 0)
        return;

    tile_size = (PLAY_BOTTOM - PLAY_TOP) / world_map.getHeight();

    auto players = state.getPlayers();
    int me = state.getMyPlayerNumber();
    roster = state.getCharacters();

    smoothPositions(players, dt);
    updateCamera(me);
    updateParticles(dt);

    for (const CollisionEffect &effect : state.takeEffects())
        addEffect(effect, me);

    // World view shakes when someone gets zapped, the HUD doesn't
    sf::View ui_view(sf::FloatRect({0, 0}, {SCREEN_WIDTH, SCREEN_HEIGHT}));
    sf::View world_view = ui_view;

    if (shake > 0) {
        world_view.move({randf(-12, 12), randf(-12, 12)});
        shake -= dt;
    }

    window.setView(world_view);
    renderDistrict(state.getDistrict());
    renderMapTiles(state.getCollectedCoins());
    renderPlayers(players, me);
    renderParticles();

    window.setView(ui_view);
    renderEffects();
    renderHUD(players, me);
    renderCountdown(phase, state.getCountdown());

    if (phase != Phase::Over && banner_clock.getElapsedTime().asSeconds() < BANNER_DURATION) {
        drawGlitchText(banner, 54, NEON_CYAN, {SCREEN_WIDTH / 2, 200});
        window.draw(makeText(banner_tagline, 28, DIM_WHITE, {SCREEN_WIDTH / 2, 260}));
    }

    // Leave time to watch the zap before the curtain falls
    if (phase == Phase::Over && phase_clock.getElapsedTime().asSeconds() > GAME_OVER_DELAY)
        renderGameOver(state, players, me);
}

void Renderer::renderCountdown(Phase phase, int countdown)
{
    sf::Vector2f center(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    if (phase == Phase::Countdown && countdown > 0)
        drawGlitchText(std::to_string(countdown), 220, NEON_YELLOW, center);
    else if (phase == Phase::Running && phase_clock.getElapsedTime().asSeconds() < 1.2f)
        drawGlitchText("BURN THE CITY", 120, NEON_RED, center);
}

void Renderer::renderGameOver(GameState &state, const std::map<int, PlayerState> &players, int me)
{
    sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
    overlay.setFillColor(sf::Color(0, 0, 0, 190));
    window.draw(overlay);

    sf::Vector2f center(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    int winner = state.getWinner();
    bool solo = players.size() == 1;
    auto mine = players.find(me);

    if (solo && mine != players.end()) {
        drawGlitchText("FLATLINED", 140, NEON_RED, center + sf::Vector2f(0, -150));
        window.draw(makeText(std::format("{} M        {} EDDIES", static_cast<int>(mine->second.x), mine->second.score),
                             44, sf::Color::White, center + sf::Vector2f(0, 10)));
        if (new_record)
            drawGlitchText("NEW PERSONAL BEST", 48, NEON_YELLOW, center + sf::Vector2f(0, 95));
        else
            window.draw(makeText(std::format("BEST  {} M", best_distance), 32, DIM_WHITE, center + sf::Vector2f(0, 95)));
    } else {
        if (winner == me)
            drawGlitchText("LEGEND OF THE AFTERLIFE", 90, NEON_YELLOW, center + sf::Vector2f(0, -150));
        else if (winner >= 0)
            drawGlitchText("FLATLINED", 140, NEON_RED, center + sf::Vector2f(0, -150));
        else
            drawGlitchText("NO WINNERS IN NIGHT CITY", 90, NEON_CYAN, center + sf::Vector2f(0, -150));

        std::string scores;
        for (const auto &[player_num, player] : players) {
            if (!scores.empty())
                scores += "        ";
            scores += playerLabel(player_num, me) + "  " + std::to_string(player.score) + " EDDIES";
        }
        window.draw(makeText(std::format("{} M", mine != players.end() ? static_cast<int>(mine->second.x) : 0),
                             44, sf::Color::White, center + sf::Vector2f(0, 10)));
        window.draw(makeText(scores, 34, DIM_WHITE, center + sf::Vector2f(0, 80)));
    }

    if (phase_clock.getElapsedTime().asSeconds() < PROMPT_DELAY)
        return;

    sf::Vector2f prompt_pos = center + sf::Vector2f(0, 220);
    if (state.isRematchRequested())
        window.draw(makeText("WAITING FOR YOUR CHOOM...", 32, NEON_CYAN, prompt_pos));
    else
        window.draw(makeText(solo ? "[SPACE] RUN AGAIN     [ESC] MENU" : "[SPACE] REMATCH     [ESC] MENU",
                             32, NEON_YELLOW, prompt_pos));
}

void Renderer::renderConnectionLost()
{
    window.setView(sf::View(sf::FloatRect({0, 0}, {SCREEN_WIDTH, SCREEN_HEIGHT})));

    sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);

    drawGlitchText("CONNECTION LOST", 110, NEON_RED, {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40});
    window.draw(makeText("[ESC] BACK TO MENU", 32, NEON_YELLOW, {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 100}));
}

//=============================================================================
// World Rendering
//=============================================================================

sf::Vector2f Renderer::toScreen(sf::Vector2f world) const
{
    return {(world.x - camera_x) * tile_size, PLAY_TOP + world.y * tile_size};
}

// The server ticks at 60Hz, easing toward its positions hides the jitter
void Renderer::smoothPositions(const std::map<int, PlayerState> &players, float dt)
{
    for (const auto &[player_num, player] : players) {
        sf::Vector2f target(player.x, player.y);
        auto [it, inserted] = display_positions.try_emplace(player_num, target);
        sf::Vector2f &pos = it->second;

        if ((target - pos).length() > 3)
            pos = target;
        else
            pos += (target - pos) * std::min(1.0f, dt * 20);
    }
}

void Renderer::updateCamera(int me)
{
    auto it = display_positions.find(me);

    if (it != display_positions.end())
        camera_x = std::max(0.0f, it->second.x - SCREEN_WIDTH / tile_size / 4);
}

// Scaled so the backdrop's ceiling and feet line land on the top and bottom of the map
void Renderer::renderBackdrop(const Backdrop &backdrop, float opacity)
{
    sf::Vector2f size(backdrop.texture.getSize());

    if (size.y == 0 || opacity <= 0)
        return;

    float scale = (PLAY_BOTTOM - PLAY_TOP) / ((backdrop.floor - backdrop.ceiling) * size.y);
    float image_top = PLAY_TOP - backdrop.ceiling * size.y * scale;
    sf::Color color = backdrop.tint;
    color.a = static_cast<uint8_t>(255 * opacity);

    for (const Backdrop::Layer &layer : backdrop.layers) {
        float y0 = image_top + layer.top * size.y * scale;
        float y1 = image_top + layer.bottom * size.y * scale;
        std::vector<Backdrop::Segment> segments = layer.segments;

        if (segments.empty()) {
            int v0 = layer.top * size.y;
            segments.push_back({0, sf::IntRect({0, v0}, {static_cast<int>(size.x), static_cast<int>(layer.bottom * size.y) - v0})});
        }

        // Every segment keeps its proportions and fills the layer's height.
        // One fade width for every seam, so each one is fully faded in when the previous one ends.
        std::vector<float> widths;
        for (const Backdrop::Segment &segment : segments)
            widths.push_back(segment.source.size.x * (y1 - y0) / segment.source.size.y);

        float fade = BG_OVERLAP * *std::min_element(widths.begin(), widths.end());
        float cycle = 0;
        for (float width : widths)
            cycle += width - fade;

        // Left to right, each picture fades in over the end of the previous one
        float x = -std::fmod(camera_x * tile_size * layer.parallax, cycle) - cycle;
        for (size_t i = 0; x < SCREEN_WIDTH; i = (i + 1) % segments.size()) {
            const Backdrop::Segment &segment = segments[i];
            float width = widths[i];

            if (x + width > 0) {
                const sf::Texture &texture = segment.texture == 0 ? backdrop.texture : backdrop.extra_textures[segment.texture - 1];
                sf::FloatRect src(segment.source);
                float u_fade = src.position.x + src.size.x * fade / width;
                float u0 = src.position.x;
                float u1 = src.position.x + src.size.x;
                float v0 = src.position.y;
                float v1 = src.position.y + src.size.y;
                sf::Color clear = color;
                clear.a = 0;
                sf::VertexArray quads(sf::PrimitiveType::Triangles);

                auto addQuad = [&](float x0, float x1, float ua, float ub, sf::Color left) {
                    quads.append({{x0, y0}, left, {ua, v0}});
                    quads.append({{x1, y0}, color, {ub, v0}});
                    quads.append({{x1, y1}, color, {ub, v1}});
                    quads.append({{x0, y0}, left, {ua, v0}});
                    quads.append({{x1, y1}, color, {ub, v1}});
                    quads.append({{x0, y1}, left, {ua, v1}});
                };
                addQuad(x, x + fade, u0, u_fade, clear);
                addQuad(x + fade, x + width, u_fade, u1, color);
                window.draw(quads, sf::RenderStates(&texture));
            }
            x += width - fade;
        }
    }
}

// BOTH WORLDS takes turns by distance, so every player sees the switch at the same spot
void Renderer::renderDistrict(int district)
{
    int shown = std::clamp(district, 0, static_cast<int>(backdrops.size()) - 1);
    int next = shown;
    float fade = 0;

    if (district == MenuLayout::DISTRICT_MIX) {
        int zone = static_cast<int>(camera_x / ZONE_LENGTH);
        float into_zone = camera_x - zone * ZONE_LENGTH;

        shown = zone % 2;
        next = (zone + 1) % 2;
        fade = std::clamp((into_zone - (ZONE_LENGTH - ZONE_FADE)) / ZONE_FADE, 0.0f, 1.0f);
    }

    renderBackdrop(backdrops[shown], 1);
    renderBackdrop(backdrops[next], fade);

    int current = fade > 0.5f ? next : shown;
    if (current != shown_backdrop) {
        shown_backdrop = current;
        banner = "ENTERING " + backdrops[current].name;
        banner_tagline = backdrops[current].tagline;
        banner_clock.restart();
    }
}

// The first far picture of a backdrop squeezed into dest; from/to pick a horizontal slice of the card
void Renderer::drawThumbnail(const Backdrop &backdrop, sf::FloatRect dest, float from, float to)
{
    sf::Vector2f size(backdrop.texture.getSize());

    if (size.y == 0 || backdrop.layers.empty())
        return;

    const Backdrop::Layer &far = backdrop.layers.front();
    const sf::Texture *texture = &backdrop.texture;
    sf::FloatRect source({0, backdrop.ceiling * size.y}, {size.x, (backdrop.floor - backdrop.ceiling) * size.y});

    if (!far.segments.empty()) {
        const Backdrop::Segment &segment = far.segments.front();
        texture = segment.texture == 0 ? &backdrop.texture : &backdrop.extra_textures[segment.texture - 1];
        source = sf::FloatRect(segment.source);
    }

    float source_w = std::min(source.size.x, source.size.y * dest.size.x / dest.size.y);
    sf::IntRect rect({static_cast<int>(source.position.x + source_w * from), static_cast<int>(source.position.y)},
                     {static_cast<int>(source_w * (to - from)), static_cast<int>(source.size.y)});

    sf::Sprite sprite(*texture);
    sprite.setTextureRect(rect);
    sprite.setScale({dest.size.x / source_w, dest.size.y / source.size.y});
    sprite.setPosition(dest.position + sf::Vector2f(dest.size.x * from, 0));
    window.draw(sprite);
}

void Renderer::renderMapTiles(const std::set<std::pair<int, int>> &collected)
{
    int start_x = std::max(0, static_cast<int>(camera_x) - 1);
    int end_x = std::min(static_cast<int>(world_map.getWidth()), static_cast<int>(camera_x + SCREEN_WIDTH / tile_size) + 2);
    float time = animation_clock.getElapsedTime().asSeconds();

    for (size_t y = 0; y < world_map.getHeight(); y++) {
        for (int x = start_x; x < end_x; x++) {
            char tile = world_map.getTile(x, y);

            if (tile == 'c' && !collected.count({x, y})) {
                sf::Vector2f world(x, y + std::sin(time * 4 + x) * 0.06f);
                sf::CircleShape halo(tile_size * 0.32f);

                halo.setOrigin({halo.getRadius(), halo.getRadius()});
                halo.setPosition(toScreen(world + sf::Vector2f(0.5f, 0.5f)));
                halo.setFillColor(sf::Color(NEON_PINK.r, NEON_PINK.g, NEON_PINK.b, 40));
                window.draw(halo, sf::BlendAdd);
                drawTileSprite(coin_texture, world, 0.55f, sf::Color::White);
            } else if (tile == 'e') {
                renderZapper(x, y);
            }
        }
    }
}

// Jetpack Joyride zapper: neighbouring nodes are linked by a live beam
void Renderer::renderZapper(int x, int y)
{
    static const sf::Vector2i links[] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    sf::Vector2f center = toScreen({x + 0.5f, y + 0.5f});
    bool linked = false;

    for (sf::Vector2i link : links) {
        if (y + link.y >= 0 && world_map.getTile(x + link.x, y + link.y) == 'e') {
            drawBeam(center, toScreen({x + link.x + 0.5f, y + link.y + 0.5f}));
            linked = true;
        }
    }

    // A lone node just crackles
    if (!linked) {
        for (int i = 0; i < 3; i++) {
            float angle = randf(0, 6.2832f);
            sf::Vector2f tip = center + sf::Vector2f(std::cos(angle), std::sin(angle)) * tile_size * randf(0.4f, 0.7f);
            drawLightning(center, tip, i == 0 ? sf::Color::White : NEON_CYAN);
        }
    }

    float pulse = 0.8f + 0.2f * std::sin(animation_clock.getElapsedTime().asSeconds() * 14 + x);
    sf::CircleShape halo(tile_size * 0.5f * pulse);
    halo.setOrigin({halo.getRadius(), halo.getRadius()});
    halo.setPosition(center);
    halo.setFillColor(sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 70));
    window.draw(halo, sf::BlendAdd);

    drawTileSprite(electric_texture, {static_cast<float>(x), static_cast<float>(y)}, 0.55f,
                   sf::Color(255, 255, 255, std::rand() % 8 ? 255 : 140));
}

void Renderer::drawBeam(sf::Vector2f from, sf::Vector2f to)
{
    sf::Vector2f delta = to - from;
    float flicker = randf(0.7f, 1);

    sf::RectangleShape glow({delta.length(), tile_size * 0.35f * flicker});
    glow.setOrigin({0, glow.getSize().y / 2});
    glow.setPosition(from);
    glow.setRotation(delta.angle());
    glow.setFillColor(sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 60));
    window.draw(glow, sf::BlendAdd);

    sf::RectangleShape core({delta.length(), 4});
    core.setOrigin({0, 2});
    core.setPosition(from);
    core.setRotation(delta.angle());
    core.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(200 * flicker)));
    window.draw(core, sf::BlendAdd);

    drawLightning(from, to, NEON_CYAN);
    drawLightning(from, to, sf::Color::White);
}

void Renderer::drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color)
{
    const int segments = 6;
    sf::Vector2f normal = (to - from).perpendicular().normalized();
    sf::VertexArray bolt(sf::PrimitiveType::LineStrip);

    for (int i = 0; i <= segments; i++) {
        float jitter = (i == 0 || i == segments) ? 0 : randf(-0.15f, 0.15f) * tile_size;
        bolt.append({from + (to - from) * (i / static_cast<float>(segments)) + normal * jitter, color});
    }
    window.draw(bolt, sf::BlendAdd);

    // Lines are 1px, a second pass makes the arc readable once the window is scaled down
    for (size_t i = 0; i < bolt.getVertexCount(); i++)
        bolt[i].position += {1.5f, 1.5f};
    window.draw(bolt, sf::BlendAdd);
}

// Fits a texture inside the tile at world position, centered
void Renderer::drawTileSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color)
{
    sf::Vector2f size(texture.getSize());

    if (size.x == 0)
        return;

    sf::Sprite sprite(texture);
    float scale = tile_size * fill / std::max(size.x, size.y);

    sprite.setOrigin(size / 2.f);
    sprite.setScale({scale, scale});
    sprite.setPosition(toScreen(world + sf::Vector2f(0.5f, 0.5f)));
    sprite.setColor(color);
    window.draw(sprite);
}

// Unknown or missing ids fall back to the first character
const Character &Renderer::characterOf(int player_num) const
{
    auto it = roster.find(player_num);
    int id = it != roster.end() ? it->second : player_num;

    return characters[std::clamp(id, 0, static_cast<int>(characters.size()) - 1)];
}

void Renderer::renderPlayers(const std::map<int, PlayerState> &players, int me)
{
    bool live = last_phase == Phase::Running;

    for (const auto &[player_num, player] : players) {
        sf::Vector2f pos = display_positions[player_num];

        // Exhaust stays in the world, so it trails behind as the player runs
        if (live && player.jet_active) {
            emitParticles(pos + sf::Vector2f(0.25f, 1.55f), 2, characterOf(player_num).exhaust, 1.5f, 0.35f, 0, {0, 8});
            emitParticles(pos + sf::Vector2f(0.25f, 1.55f), 1, NEON_ORANGE, 2.5f, 0.25f, 0, {0, 6});
        }
    }

    // Others first so you are always drawn on top
    for (const auto &[player_num, player] : players) {
        if (player_num != me)
            renderPlayerSprite(player_num, player, false);
    }

    auto mine = players.find(me);
    if (mine != players.end())
        renderPlayerSprite(me, mine->second, true);
}

// Animation follows what the player does: run, take off, rise, hover at the ceiling, fall, die
void Renderer::renderPlayerSprite(int player_num, const PlayerState &player, bool is_me)
{
    const Character &character = characterOf(player_num);
    sf::Vector2f pos = display_positions[player_num];
    bool on_ground = pos.y >= world_map.getHeight() - PLAYER_HEIGHT - 0.01f;
    PlayerAnim &state = player_anims[player_num];

    if (on_ground) {
        state.on_ground = true;
    } else if (state.on_ground) {
        state.on_ground = false;
        state.air_clock.restart();
    }

    auto death = death_clocks.find(player_num);
    bool dead = death != death_clocks.end();
    float air_time = state.air_clock.getElapsedTime().asSeconds();
    float time = animation_clock.getElapsedTime().asSeconds();
    const Animation *anim = &character.fall;

    if (dead && character.death.frames > 0) {
        anim = &character.death;
        time = death->second.getElapsedTime().asSeconds();
    } else if (on_ground) {
        anim = &character.run;
    } else if (air_time < character.takeoff.duration()) {
        anim = &character.takeoff;
        time = air_time;
    } else if (player.jet_active) {
        anim = pos.y <= 0.01f ? &character.hover : &character.rise;
    }

    int frame = anim->frameAt(time);
    if (frame < 0 || character.sheet.getSize().x == 0)
        return;

    // Electrocution: shaking and flashing, for good if there is no death animation
    bool shocked = dead && (character.death.frames == 0 || death->second.getElapsedTime().asSeconds() < 0.4f);
    sf::Vector2f feet = toScreen(pos + sf::Vector2f(0.5f, PLAYER_HEIGHT));
    float scale = tile_size * PLAYER_HEIGHT * 1.05f / character.body;

    if (shocked)
        feet += {randf(-6, 6), randf(-6, 6)};

    sf::Sprite sprite(character.sheet);
    sprite.setTextureRect(character.frameRect(*anim, frame));
    sprite.setOrigin({character.frame_size.x / 2.f, character.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(feet);

    if (shocked)
        sprite.setColor(std::rand() % 2 ? NEON_CYAN : sf::Color::White);
    else if (!is_me)
        sprite.setColor(sf::Color(255, 255, 255, 140));
    window.draw(sprite);

    if (shocked && std::rand() % 2)
        window.draw(sprite, sf::BlendAdd);

    window.draw(makeText(is_me ? "YOU" : "CHOOM", 22, is_me ? NEON_YELLOW : NEON_CYAN,
                         feet - sf::Vector2f(0, tile_size * (PLAYER_HEIGHT + 0.25f))));
}

//=============================================================================
// Effects, Particles & HUD
//=============================================================================

void Renderer::addEffect(const CollisionEffect &effect, int me)
{
    sf::Vector2f tile_center(effect.x + 0.5f, effect.y + 0.5f);

    if (effect.type == 'e') {
        shake = 0.5f;
        death_clocks.try_emplace(effect.player);
        emitParticles(tile_center, 40, NEON_CYAN, 9, 0.7f, 12);
        emitParticles(tile_center, 20, sf::Color::White, 6, 0.5f, 12);
    } else if (effect.player == me) {
        emitParticles(tile_center, 14, NEON_PINK, 5, 0.45f, 0);
        emitParticles(tile_center, 6, NEON_YELLOW, 3, 0.35f, 0);
    } else {
        return;
    }
    effects.push_back({effect, sf::Clock()});
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
            drawTileSprite(coin_texture, tile, 0.55f + t, sf::Color(255, 255, 255, alpha));

            sf::Color color = NEON_CYAN;
            color.a = alpha;
            window.draw(makeText("+1", 30, color, toScreen(tile + sf::Vector2f(0.5f, -t))));
        } else {
            sf::RectangleShape flash({SCREEN_WIDTH, SCREEN_HEIGHT});
            flash.setFillColor(sf::Color(NEON_RED.r, NEON_RED.g, NEON_RED.b, alpha / 2));
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
        sf::Color color = p.color;
        color.a = static_cast<uint8_t>(255 * p.life / p.max_life);

        shape.setSize({p.size, p.size});
        shape.setOrigin({p.size / 2, p.size / 2});
        shape.setPosition(toScreen(p.pos));
        shape.setFillColor(color);
        window.draw(shape, sf::BlendAdd);
    }
}

void Renderer::renderHUD(const std::map<int, PlayerState> &players, int me)
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
        const sf::Texture &portrait = characterOf(player_num).portrait;
        if (portrait.getSize().y > 0) {
            sf::Sprite icon(portrait);
            float s = 34.f / portrait.getSize().y;
            icon.setScale({s, s});
            icon.setPosition({24, y - 2});
            window.draw(icon);
        }

        std::string line = playerLabel(player_num, me) + "   " + std::to_string(player.score) + " EDDIES";
        window.draw(makeText(line, 28, player_num == me ? NEON_YELLOW : NEON_CYAN, {70, y}, {0, 0}));
        y += 44;
    }
}

//=============================================================================
// Helper Functions
//=============================================================================

bool Renderer::blink() const
{
    return static_cast<int>(animation_clock.getElapsedTime().asSeconds() * 2) % 2 == 0;
}

sf::Text Renderer::makeText(const std::string &content, unsigned int size, sf::Color color,
                            sf::Vector2f pos, sf::Vector2f anchor)
{
    bool needs_fallback = content.find_first_of(missing_glyphs) != std::string::npos;
    sf::Text text(needs_fallback ? fallback_font : font, content, size);
    sf::FloatRect bounds = text.getLocalBounds();

    text.setFillColor(color);
    text.setOutlineColor(sf::Color(0, 0, 0, color.a));
    text.setOutlineThickness(size / 16.f);
    text.setOrigin(bounds.position + bounds.size.componentWiseMul(anchor));
    text.setPosition(pos);
    return text;
}

// Chromatic split, with the odd jolt
void Renderer::drawGlitchText(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos)
{
    float offset = size / 30.f * (std::rand() % 25 == 0 ? 4 : 1);
    sf::Text text = makeText(content, size, color, pos);

    text.setOutlineThickness(0);
    text.setFillColor(sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 170));
    text.setPosition(pos + sf::Vector2f(-offset, 0));
    window.draw(text);

    text.setFillColor(sf::Color(NEON_RED.r, NEON_RED.g, NEON_RED.b, 170));
    text.setPosition(pos + sf::Vector2f(offset, 0));
    window.draw(text);

    window.draw(makeText(content, size, color, pos));
}

std::string Renderer::playerLabel(int player_num, int me) const
{
    return player_num == me ? "YOU" : "CHOOM";
}
