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
    : client(client), camera_x(0), show_countdown(false), countdown_value(0) {
}

Renderer::~Renderer() {
    if (window.isOpen()) {
        window.close();
    }
}

//=============================================================================
// Initialization
//=============================================================================

bool Renderer::initialize() {
    if (!createWindow()) {
        return false;
    }

    loadAssets();
    return true;
}

bool Renderer::createWindow()
{
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Jetpack Game"); // A MODIF DANS HPP
    window.setFramerateLimit(60);
    return window.isOpen();
}

void Renderer::loadAssets() {
    loadTexture(background_texture, "assets/background/background.png", "background");
    loadTexture(player_texture, "assets/johny/Xjohny.png", "player");
    loadTexture(jetpack_texture, "assets/Xjetpack.png", "jetpack");
    loadTexture(coin_texture, "assets/coins/Xcoin.png", "coin");
    loadTexture(electric_texture, "assets/electric/Xzap.png", "electric");

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

    window.clear(sf::Color(50, 50, 150));

    GameState* state = client->getGameState();

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

void Renderer::updateCamera(GameState *state)
{
    auto players = state->getPlayers();
    int my_player_num = client->getPlayerNumber();

    if (players.count(my_player_num) > 0) {
        camera_x = players[my_player_num].x - SCREEN_WIDTH / (2 * TILE_SIZE);
        if (camera_x < 0) camera_x = 0;
    }
}

void Renderer::renderGameScreen(GameState *state)
{
    renderMap(client->getMap());
    renderPlayers(state);
    renderEffects(state);
    renderHUD(state);

    if (client->isGameOver()) {
        renderGameOver(state);
    }
}

void Renderer::renderWaitingScreen() {
    if (show_countdown) {
        sf::Text countText;
        setupText(countText, std::to_string(countdown_value), 100, sf::Color::White);
        centerText(countText, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        window.draw(countText);

        // Hide countdown after 1 second
        auto elapsed = std::chrono::steady_clock::now() - countdown_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1000) {
            show_countdown = false;
        }
    } else {
        sf::Text waitText;
        setupText(waitText, "Wake the fuck up...", 40, sf::Color::Black);
        centerText(waitText, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        window.draw(waitText);
    }
}

//=============================================================================
// Map Rendering
//=============================================================================

void Renderer::renderMap(const Map& map)
{
    renderBackground();
    renderMapTiles(map);
}

void Renderer::renderBackground()
{
    if (background_texture.getSize().x > 0) {
        sf::Sprite background(background_texture);

        float scale_x = static_cast<float>(SCREEN_WIDTH) / background_texture.getSize().x;
        float scale_y = static_cast<float>(SCREEN_HEIGHT) / background_texture.getSize().y;

        background.setScale(scale_x, scale_y);
        window.draw(background);
    }
}

void Renderer::renderMapTiles(const Map &map)
{
    int start_x = static_cast<int>(camera_x);
    int end_x = start_x + SCREEN_WIDTH / TILE_SIZE + 1;

    size_t map_width = map.getWidth();
    size_t map_height = map.getHeight();

    if (static_cast<size_t>(end_x) > map_width) {
        end_x = static_cast<int>(map_width);
    }

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
    if (coin_texture.getSize().x > 0) {
        sf::Sprite coin(coin_texture);
        coin.setPosition(x, y);
        float scale = static_cast<float>(TILE_SIZE) / coin_texture.getSize().x;
        coin.setScale(scale, scale);
        window.draw(coin);
    } else {
        sf::RectangleShape coinShape;
        coinShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
        coinShape.setPosition(x, y);
        coinShape.setFillColor(sf::Color::Yellow);
        window.draw(coinShape);
    }
}

void Renderer::renderElectric(float x, float y)
{
    if (electric_texture.getSize().x > 0) {
        sf::Sprite electric(electric_texture);
        electric.setPosition(x, y);
        float scale = static_cast<float>(TILE_SIZE) / electric_texture.getSize().x;
        electric.setScale(scale, scale);
        window.draw(electric);
    } else {
        sf::RectangleShape electricShape;
        electricShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
        electricShape.setPosition(x, y);
        electricShape.setFillColor(sf::Color::Red);
        window.draw(electricShape);
    }
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

        // Calculate screen position
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
    if (player_texture.getSize().x > 0) {
        renderPlayerSprite(player, player_num, x, y, my_player_num);
    } else {
        renderPlayerFallback(player_num, x, y, my_player_num);
    }
}

//=============================================================================================
// TENTATIVE D ANIMATION A CHANGER DUCP CA MARCHE PAS
//=============================================================================================

void Renderer::renderPlayerSprite(const PlayerState &player, int player_num, float x, float y, int my_player_num)
{
    sf::Sprite playerSprite(player_texture);

    int frame = getCurrentAnimationFrame(player.jet_active);
    int frame_width = player_texture.getSize().x / 4; // 4 frames per row

    // Set texture rectangle based on animation
    if (player.jet_active) {
        // Jetpack animation (second row)
        playerSprite.setTextureRect(sf::IntRect(frame * frame_width, TILE_SIZE, frame_width, TILE_SIZE));
    } else {
        // Walking animation (first row)
        playerSprite.setTextureRect(sf::IntRect(frame * frame_width, 0, frame_width, TILE_SIZE));
    }

    playerSprite.setPosition(x, y);
    float scale = static_cast<float>(TILE_SIZE) / frame_width;
    playerSprite.setScale(scale, scale);

    // Highlight my player
    if (player_num == my_player_num) {
        playerSprite.setColor(sf::Color(255, 255, 255));
    } else {
        playerSprite.setColor(sf::Color(200, 200, 200));
    }
    window.draw(playerSprite);
}

void Renderer::renderPlayerFallback(int player_num, float x, float y, int my_player_num)
{
    // Fallback to a rectangle if no texture is loaded
    sf::RectangleShape playerShape;

    playerShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    playerShape.setPosition(x, y);
    playerShape.setFillColor(player_num == my_player_num ? sf::Color(0, 255, 0) : sf::Color(255, 0, 0));
    window.draw(playerShape);
}

void Renderer::renderJetpack(float x, float y)
{
    if (jetpack_texture.getSize().x > 0) {
        sf::Sprite jetpackSprite(jetpack_texture);
        jetpackSprite.setPosition(x - TILE_SIZE / 2, y);
        float scale = static_cast<float>(TILE_SIZE) / jetpack_texture.getSize().x;
        jetpackSprite.setScale(scale, scale);
        window.draw(jetpackSprite);
    } else {
        sf::RectangleShape jetpackShape;
        jetpackShape.setSize(sf::Vector2f(TILE_SIZE / 2, TILE_SIZE / 2));
        jetpackShape.setPosition(x - TILE_SIZE / 2, y + TILE_SIZE / 2);
        jetpackShape.setFillColor(sf::Color(255, 165, 0));
        window.draw(jetpackShape);
    }
}

//=============================================================================
// Effects Rendering
//=============================================================================

void Renderer::renderEffects(GameState *state)
{
//    auto effects = state->getEffects();
//
//    for (const auto& effect : effects) {
//        float x = (effect.x - camera_x) * TILE_SIZE;
//        float y = effect.y * TILE_SIZE;
//
//        renderEffect(effect, x, y);
//    }
//    state->updateEffects();
}

void Renderer::renderEffect(const GameState::CollisionEffect& effect, float x, float y)
{
//    sf::CircleShape effectShape;
//    effectShape.setRadius(TILE_SIZE / 2);
//    effectShape.setPosition(x, y);
//
//    // Fade out based on lifetime
//    int alpha = 255 * effect.lifetime / 20;
//
//    if (effect.type == 'c') {
//        // Coin effect
//        effectShape.setFillColor(sf::Color(255, 255, 0, alpha));
//    } else if (effect.type == 'e') {
//        // Electric hazard effect
//        effectShape.setFillColor(sf::Color(255, 0, 0, alpha));
//    }
//
//    window.draw(effectShape);
}

//=============================================================================
// HUD and UI Rendering
//=============================================================================

void Renderer::renderHUD(GameState *state)
{
    auto players = state->getPlayers();
    int my_player_number = client->getPlayerNumber();

    // Create a simple score display for all players
    int y_offset = 10;  // Start at the top with some margin

    // Display all player scores in a clean, simple format
    for (const auto& player_pair : players) {
        int player_num = player_pair.first;
        const PlayerState& player = player_pair.second;

        sf::Text score_text;
        score_text.setFont(font);

        // Format: "Player X: 000" or "You: 000" for the local player
        std::string player_label = (player_num == my_player_number) ? "You" : "Player " + std::to_string(player_num);
        score_text.setString(player_label + ": " + std::to_string(player.score));

        // Set text size and color (highlight local player's score)
        score_text.setCharacterSize(24);
        if (player_num == my_player_number) {
            score_text.setFillColor(sf::Color::White);
            score_text.setStyle(sf::Text::Bold);
        } else {
            score_text.setFillColor(sf::Color(200, 200, 200));  // Light gray for other players
        }

        // Position the text
        score_text.setPosition(10, y_offset);
        window.draw(score_text);

        // Increment vertical position for next score text
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
    setupText(game_over_text, "GAME OVER", 50, sf::Color::White);
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
    sf::Text scores_text;
    scores_text.setFont(font);

    // Collect all player scores
    std::string score_string = "Final Scores:";
    auto players = state->getPlayers();
    for (const auto& player_pair : players) {
        int player_num = player_pair.first;
        const PlayerState& player = player_pair.second;

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
    setupText(exit_text, "Press ESC to get the fuck out of here, you fucking loser", 20, sf::Color(200, 200, 200));
    centerText(exit_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 150);
    window.draw(exit_text);
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

int Renderer::getCurrentAnimationFrame(bool) const
{
    float elapsed = animation_clock.getElapsedTime().asSeconds();
    return static_cast<int>(elapsed / ANIMATION_FRAME_DURATION) % 4;
}

void Renderer::setupText(sf::Text& text, const std::string& content, unsigned int size, const sf::Color& color) {
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
