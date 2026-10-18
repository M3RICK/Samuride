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
#include <cstdlib>

float randf(float min, float max)
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
    sf::Vector2u size(SCREEN_WIDTH * fit, SCREEN_HEIGHT * fit);

    window.create(sf::VideoMode(size), "Samuride", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
}

void Renderer::loadAssets()
{
    loadBackdrops();
    loadCharacters();
    loadFonts();

    loadTexture(waiting_screen_texture, "assets/waiting_screen/Samuride.png");
    loadTexture(coin_texture, "assets/coins/coin.png");
    loadTexture(electric_texture, "assets/electric/zap.png");
}

// The corridor scrolls as one layer; the others run on the Worlds Collide walkway, a panorama far behind
void Renderer::loadBackdrops()
{
    const sf::Texture *corridor = sharedTexture("assets/background/Resized.png");
    const sf::Texture *walkway = sharedTexture("assets/background/worlds_collide.png");
    const sf::Texture *citadel = sharedTexture("assets/background/neon_citadel.png");

    // neon_citadel.png stacks three panoramas, each cut just above its own (blurry) walkway
    int citadel_width = citadel->getSize().x;
    Backdrop::Segment night_city = {citadel, sf::IntRect({0, 0}, {citadel_width, 260})};
    Backdrop::Segment walls = {citadel, sf::IntRect({0, 283}, {citadel_width, 209})};
    Backdrop::Segment zaun = {citadel, sf::IntRect({0, 513}, {citadel_width, 180})};

    auto onWalkway = [&](const std::string &name, const std::string &tagline,
                         std::vector<Backdrop::Segment> far, float parallax) {
        // Dimmed a little: the neons would swallow the coins and zappers
        return Backdrop{name, tagline, walkway, 0.05f, 0.825f, sf::Color(200, 200, 215),
                        {{0, 0.8f, parallax, far}, {0.8f, 1, 1, {}}}};
    };

    backdrops = {
        {"THE CORRIDOR", "Arasaka sublevel 3", corridor, 0.20f, 0.79f, sf::Color::White, {{0, 1, 0.4f, {}}}},
        // The three panoramas back to back, a bit faster so a run goes through them
        onWalkway("WORLDS COLLIDE", "Night City, the Walls & Zaun", {night_city, walls, zaun}, 0.25f),
        onWalkway("NIGHT CITY", "Neon towers at sunset", {night_city}, 0.2f),
        onWalkway("THE WALLS", "Dusk along the Walls", {walls}, 0.2f),
        onWalkway("ZAUN", "The undercity never sleeps", {zaun}, 0.2f),
    };
}

// Pixel art: character textures stay unsmoothed so they stay crisp when scaled up
void Renderer::loadCharacters()
{
    characters.resize(CHARACTER_DIRS.size());

    for (size_t i = 0; i < CHARACTER_DIRS.size(); i++) {
        if (!characters[i].load("assets/" + CHARACTER_DIRS[i]))
            DEBUG_LOG("Failed to load character: " + CHARACTER_DIRS[i]);
    }
}

void Renderer::loadFonts()
{
    if (!font.openFromFile("assets/font/jetpack_font.ttf") || !fallback_font.openFromFile("assets/font/Rajdhani-Bold.ttf"))
        DEBUG_LOG("Failed to load fonts, text will not be displayed");

    for (char32_t c = '!'; c <= '~'; c++) {
        if (font.getGlyph(c, 32, false).bounds.size.x == 0)
            missing_glyphs += static_cast<char>(c);
    }
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

void Renderer::loadTexture(sf::Texture &texture, const std::string &path)
{
    if (!texture.loadFromFile(path)) {
        DEBUG_LOG("Failed to load texture: " + path);
        return;
    }
    texture.setSmooth(true);
}

const sf::Texture *Renderer::sharedTexture(const std::string &path)
{
    auto [it, inserted] = textures.try_emplace(path);

    if (inserted)
        loadTexture(it->second, path);
    return &it->second;
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

void Renderer::onPhaseChange(GameState &state, Phase phase)
{
    last_phase = phase;
    phase_clock.restart();

    if (phase == Phase::Countdown)
        resetRun();
    if (phase == Phase::Over)
        recordSoloDistance(state);
}

// New run (first one or rematch): forget the previous one
void Renderer::resetRun()
{
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
void Renderer::recordSoloDistance(GameState &state)
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

void Renderer::drawOverlay(uint8_t alpha)
{
    sf::RectangleShape overlay({SCREEN_WIDTH, SCREEN_HEIGHT});

    overlay.setFillColor(sf::Color(0, 0, 0, alpha));
    window.draw(overlay);
}

// Dark panel used by the selection screens, outlined in yellow when selected
void Renderer::drawCard(sf::FloatRect box, bool selected)
{
    sf::RectangleShape card(box.size);

    card.setPosition(box.position);
    card.setFillColor(sf::Color(10, 8, 20, 235));
    card.setOutlineColor(selected ? NEON_YELLOW : sf::Color(NEON_CYAN.r, NEON_CYAN.g, NEON_CYAN.b, 90));
    card.setOutlineThickness(selected ? 5 : 2);
    window.draw(card);
}

std::string Renderer::playerLabel(int player_num, int me) const
{
    return player_num == me ? "YOU" : "CHOOM";
}

// Unknown or missing ids fall back to the first character
const Character &Renderer::characterOf(int player_num) const
{
    auto it = roster.find(player_num);
    int id = it != roster.end() ? it->second : player_num;

    return characters[std::clamp(id, 0, static_cast<int>(characters.size()) - 1)];
}
