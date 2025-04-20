/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include "client.hpp"
#include "state.hpp"
#include "../common/debug.hpp"
#include <sstream>
#include <iomanip>


Renderer::Renderer(Client* client)
    : client(client), camera_x(0), show_countdown(false), countdown_value(3),
      background_offset(1.0f), scroll_speed(0.5f) {
}

Renderer::~Renderer() {
    if (window.isOpen()) {
        window.close();
    }
}

//=============================================================================
// Initialization
//=============================================================================

bool Renderer::initialize()
{
    if (!createWindow())
        return false;

    loadAssets();
    loadAudio();
    return true;
}

bool Renderer::createWindow()
{
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Jetpack Game"); // A MODIF DANS HPP
    window.setFramerateLimit(60);
    return window.isOpen();
}


// ENLEVER X POUR GOOD PATH (!FALLBACK)
void Renderer::loadAssets() {
    loadTexture(background_texture, "assets/background/Resized.png", "background");
    loadTexture(waiting_screen_texture, "assets/waiting_screen/Samuride.png", "waiting_screen");
    loadTexture(johny_texture, "assets/johny/johny.png", "player");
    loadTexture(david_texture, "assets/david/david.png", "player2");
    loadTexture(coin_texture, "assets/coins/coin.png", "coin");
    loadTexture(electric_texture, "assets/electric/zap.png", "electric");

    loadFont();
}

void Renderer::loadTexture(sf::Texture &texture, const std::string &path, const std::string &name)
{
    if (!texture.loadFromFile(path)) {
        DEBUG_LOG("Failed to load " + name + " texture, using fallback");
    }
}

void Renderer::loadFont()
{
    if (!font.loadFromFile("assets/font/jetpack_font.ttf")) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            DEBUG_LOG("Failed to load font, text may not be displayed properly");
        }
    }
}

//=============================================================================
// Main Render Loop
//=============================================================================

void Renderer::render()
{
    if (!client)
        return;

    window.clear(sf::Color::Black);

    GameState *state = client->getGameState();

    if (!state)
        return;

    updateCamera(state);

    if (client->isGameStarted()) {
        renderGameScreen(state);
    } else {
        renderWaitingScreen();
    }

    window.display();
}

void Renderer::updateCamera(GameState* state)
{
    auto players = state->getPlayers();
    int my_player_num = client->getPlayerNumber();

    if (players.count(my_player_num) > 0) {
        float target_x = players[my_player_num].x - SCREEN_WIDTH / (2 * TILE_SIZE);
        camera_x += (target_x - camera_x) * 0.1f;
        if (camera_x < 0)
            camera_x = 0;
    }
}

void Renderer::renderGameScreen(GameState *state)
{
    if (background_music.getStatus() != sf::Music::Playing) {
        waiting_music.stop();
        background_music.play();
    }

    renderMap(client->getMap());
    renderPlayers(state);
    renderEffects(state);
    renderHUD(state);

    if (client->isGameOver()) {
        renderGameOver(state);
    }
}

void Renderer::renderWaitingScreen()
{
    if (waiting_music.getStatus() != sf::Music::Playing) {
        background_music.stop();
        waiting_music.play();
    }

    if (waiting_screen_texture.getSize().x > 0) {
        sf::Sprite waitingScreenSprite(waiting_screen_texture);

        float scale_x = static_cast<float>(SCREEN_WIDTH) / waiting_screen_texture.getSize().x;
        float scale_y = static_cast<float>(SCREEN_HEIGHT) / waiting_screen_texture.getSize().y;
        float scale = std::min(scale_x, scale_y);

        waitingScreenSprite.setScale(scale, scale);

        float x_pos = (SCREEN_WIDTH - (waiting_screen_texture.getSize().x * scale)) / 2;
        float y_pos = (SCREEN_HEIGHT - (waiting_screen_texture.getSize().y * scale)) / 2;

        waitingScreenSprite.setPosition(x_pos, y_pos);
        window.draw(waitingScreenSprite);

        sf::Text waitText;
        setupText(waitText, "Wake the fuck up...", 80, sf::Color::Red);
        centerText(waitText, SCREEN_WIDTH / 2, SCREEN_HEIGHT/ 11);
        window.draw(waitText);
    }
}

//=============================================================================
// Map Rendering
//=============================================================================

void Renderer::renderMap(const Map &map)
{
    renderBackground();
    renderMapTiles(map);
}

void Renderer::renderBackground()
{
    if (background_texture.getSize().x == 0)
        return;

    background_offset = camera_x * scroll_speed;

    float bg_width = static_cast<float>(background_texture.getSize().x);
    int repetitions = static_cast<int>(SCREEN_WIDTH / bg_width) + 2;

    for (int i = 0; i < repetitions; i++) {
        sf::Sprite background(background_texture);

        float x_pos = i * bg_width - fmodf(background_offset, bg_width);
        background.setPosition(x_pos, 0);

        float scale_y = static_cast<float>(SCREEN_HEIGHT) / background_texture.getSize().y;
        background.setScale(1.0f, scale_y);

        window.draw(background);
    }
}

void Renderer::renderMapTiles(const Map &map)
{
    int start_x = static_cast<int>(camera_x);
    int end_x = start_x + SCREEN_WIDTH / TILE_SIZE + 1;

    size_t map_width = map.getWidth();
    size_t map_height = map.getHeight();

    if (static_cast<size_t>(end_x) > map_width)
        end_x = static_cast<int>(map_width);

    for (size_t y = 0; y < map_height; y++) {
        for (int x = start_x; x < end_x; x++) {
            char tile = map.getTile(x, y);
            renderTile(tile, x, y);
        }
    }
}

void Renderer::renderTile(char tile, int x, int y)
{
    float screen_x = (x - camera_x) * TILE_SIZE;
    float screen_y = y * TILE_SIZE;

    switch (tile) {
        case 'c':
            renderCoin(screen_x, screen_y);
            break;
        case 'e':
            renderElectric(screen_x, screen_y);
            break;
        default:
            break;
    }
}

void Renderer::renderCoin(float x, float y)
{
    sf::Sprite coin(coin_texture);
    coin.setPosition(x, y);
    float scale = static_cast<float>(TILE_SIZE) / coin_texture.getSize().x;
    coin.setScale(scale, scale);
    window.draw(coin);

}

void Renderer::renderElectric(float x, float y)
{
    sf::Sprite electric(electric_texture);
    electric.setPosition(x, y);
    float scale = static_cast<float>(TILE_SIZE) / electric_texture.getSize().x;
    electric.setScale(scale, scale);
    window.draw(electric);
}

//=============================================================================
// Player Rendering
//=============================================================================

void Renderer::renderPlayers(GameState *state)
{
    auto players = state->getPlayers();
    int my_player_num = client->getPlayerNumber();

    for (auto& pair : players) {
        int player_num = pair.first;
        const PlayerState& player = pair.second;

        float x = (player.x - camera_x) * TILE_SIZE;
        float y = player.y * TILE_SIZE;

        renderPlayer(player, player_num, x, y, my_player_num);

        if (player.jet_active) {
            renderJetpack(x, y);
        }
    }
}

void Renderer::renderPlayer(const PlayerState& player, int player_num, float x, float y, int my_player_num)
{
    renderPlayerSprite(player, player_num, x, y, my_player_num);
}

//=============================================================================================
//  ANIMATIONS
//=============================================================================================

void Renderer::renderPlayerSprite(const PlayerState &player, int player_num, float x, float y, int my_player_num)
{
    sf::Texture &playerTexture = (player_num == 0) ? johny_texture : david_texture;
    sf::Sprite playerSprite(playerTexture);

    const int SPRITE_WIDTH = playerTexture.getSize().x / 4;
    const int SPRITE_HEIGHT = playerTexture.getSize().y / 3;

    if (playerTexture.getSize().x == 0)
        return;


    int row = 0;
    int frame = getCurrentAnimationFrame(player.jet_active);

    if (player.jet_active) {
        row = 2;
    } else {
        row = 0;
    }

    playerSprite.setTextureRect(sf::IntRect(
        frame *SPRITE_WIDTH,
        row *SPRITE_HEIGHT,
        SPRITE_WIDTH,
        SPRITE_HEIGHT
    ));

    playerSprite.setPosition(x, y - (SPRITE_HEIGHT - TILE_SIZE));
    float scale = static_cast<float>(TILE_SIZE) / SPRITE_WIDTH;
    playerSprite.setScale(scale, scale);

    window.draw(playerSprite);
}

void Renderer::renderPlayerFallback(int player_num, float x, float y, int my_player_num)
{
    sf::RectangleShape playerShape;

    playerShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    playerShape.setPosition(x, y);
    playerShape.setFillColor(player_num == my_player_num ? sf::Color(0, 255, 0) : sf::Color(255, 0, 0));

    window.draw(playerShape);
}

void Renderer::renderJetpack(float x, float y)
{
    // c est compris dans le sprite original
}

//=============================================================================
// Effects Rendering
//=============================================================================

void Renderer::renderEffects(GameState *state)
{
    // Pareil hein
}

void Renderer::renderEffect(const GameState::CollisionEffect& effect, float x, float y)
{
    // J aurais pas le tps
}

//=============================================================================
// HUD and UI Rendering
//=============================================================================

void Renderer::renderHUD(GameState *state)
{
    auto players = state->getPlayers();
    int my_player_number = client->getPlayerNumber();
    int y_offset = 10;

    for (const auto& player_pair : players) {
        int player_num = player_pair.first;
        const PlayerState& player = player_pair.second;

        sf::Text score_text;
        score_text.setFont(font);

        std::string player_label = (player_num == my_player_number) ? "You" : "Player " + std::to_string(player_num);
        score_text.setString(player_label + ": " + std::to_string(player.score));
        score_text.setCharacterSize(24);

        if (player_num == my_player_number) {
            score_text.setFillColor(sf::Color::White);
            score_text.setStyle(sf::Text::Bold);
        } else {
            score_text.setFillColor(sf::Color::Yellow);
        }

        score_text.setPosition(10, y_offset);
        window.draw(score_text);
        y_offset += 30;
    }
}

void Renderer::renderGameOver(GameState *state)
{
    renderGameOverBackground();
    renderGameOverTitle();
    renderWinnerText(state);
    renderFinalScores(state);
    renderExitInstructions();
}

void Renderer::renderGameOverBackground()
{
    sf::RectangleShape overlay(sf::Vector2f(SCREEN_WIDTH, SCREEN_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(overlay);
}

void Renderer::renderGameOverTitle()
{
    sf::Text game_over_text;

    setupText(game_over_text, "THIS IS THE END", 50, sf::Color::White);
    centerText(game_over_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 50);

    window.draw(game_over_text);
}

void Renderer::renderWinnerText(GameState *state)
{
    int winner = state->getWinner();
    sf::Text winner_text;

    if (winner == client->getPlayerNumber()) {
        setupText(winner_text, "You Win!", 30, sf::Color::Green);
    } else if (winner >= 0) {
        setupText(winner_text, "Player " + std::to_string(winner) + " Wins!", 30, sf::Color::Red);
    } else {
        setupText(winner_text, "No Winner", 30, sf::Color::Yellow);
    }

    centerText(winner_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
    window.draw(winner_text);
}

void Renderer::renderFinalScores(GameState *state)
{
    int player_num;
    sf::Text scores_text;
    scores_text.setFont(font);

    std::string score_string = "Final Scores:";
    auto players = state->getPlayers();

    for (const auto& player_pair : players) {
        player_num = player_pair.first;
        const PlayerState &player = player_pair.second;

        std::string player_label = (player_num == client->getPlayerNumber()) ? "You" : "Player " + std::to_string(player_num);
        score_string += "\n" + player_label + ": " + std::to_string(player.score);
    }

    setupText(scores_text, score_string, 20, sf::Color::White);
    centerText(scores_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 80);
    window.draw(scores_text);
}

void Renderer::renderExitInstructions()
{
    sf::Text exit_text;

    setupText(exit_text, "Press ESC to get the fuck out of here, you fucking loser", 75, sf::Color::Red);
    centerText(exit_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150);

    window.draw(exit_text);
}

//=============================================================================
// AUDIO
//=============================================================================

void Renderer::loadAudio()
{
    if (!waiting_music.openFromFile("assets/music/waiting_screen.wav")) {
        DEBUG_LOG("Failed to load waiting music");
    } else {
        waiting_music.setLoop(false);
        waiting_music.setVolume(50);
    }

    if (!background_music.openFromFile("assets/music/ingame.wav")) {
        DEBUG_LOG("Failed to load background music");
    } else {
        background_music.setLoop(true);
        background_music.setVolume(40);
    }
}

//=============================================================================
// Helper Functions
//=============================================================================

void Renderer::setCountdown(int value)
{
    show_countdown = (value > 0);
    countdown_value = value;
    countdown_time = std::chrono::steady_clock::now();
}

int Renderer::getCurrentAnimationFrame(bool jet_active) const
{
    float elapsed = animation_clock.getElapsedTime().asSeconds();
    float frame_duration = jet_active ? 0.08f : 0.15f;
    int num_frames = 4;

    return static_cast<int>((elapsed / frame_duration)) % num_frames;
}

void Renderer::setupText(sf::Text &text, const std::string &content, unsigned int size, const sf::Color &color)
{
    text.setFont(font);
    text.setString(content);
    text.setCharacterSize(size);
    text.setFillColor(color);
}

void Renderer::centerText(sf::Text& text, float x, float y)
{
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    text.setPosition(x, y);
}
