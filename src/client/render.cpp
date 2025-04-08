#include "render.hpp"
#include "client.hpp"
#include "state.hpp"
#include "../common/debug.hpp"
#include <sstream>
#include <iomanip>

Renderer::Renderer(Client* client)
    : client(client), camera_x(0) {
}

Renderer::~Renderer() {
    if (window.isOpen()) {
        window.close();
    }
}

bool Renderer::initialize() {
    // Create window
    window.create(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Jetpack Game");
    window.setFramerateLimit(60);

    // Load assets
    if (!background_texture.loadFromFile("assets/background.png")) {
        // Fallback to a colored rectangle if texture is missing
        DEBUG_LOG("Failed to load background texture, using fallback");
    }

    if (!player_texture.loadFromFile("assets/player.png")) {
        DEBUG_LOG("Failed to load player texture, using fallback");
    }

    if (!jetpack_texture.loadFromFile("assets/jetpack.png")) {
        DEBUG_LOG("Failed to load jetpack texture, using fallback");
    }

    if (!coin_texture.loadFromFile("assets/coin.png")) {
        DEBUG_LOG("Failed to load coin texture, using fallback");
    }

    if (!electric_texture.loadFromFile("assets/electric.png")) {
        DEBUG_LOG("Failed to load electric texture, using fallback");
    }

    if (!font.loadFromFile("assets/font.ttf")) {
        // Try system font as fallback
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            DEBUG_LOG("Failed to load font, text may not be displayed properly");
        }
    }

    return true;
}

void Renderer::render() {
    if (!client) return;

    window.clear(sf::Color(50, 50, 150));

    GameState* state = client->getGameState();
    if (!state) return;

    // Update camera to follow player
    auto players = state->getPlayers();
    int my_player_num = client->getPlayerNumber();
    if (players.count(my_player_num) > 0) {
        camera_x = players[my_player_num].x - SCREEN_WIDTH / (2 * TILE_SIZE);
        if (camera_x < 0) camera_x = 0;
    }

    // Render game elements
    if (client->isGameStarted()) {
        renderMap(client->getMap());
        renderPlayers(state);
        renderEffects(state);
        renderHUD(state);

        if (client->isGameOver()) {
            renderGameOver(state);
        }
    } else {
        // Render waiting screen
        sf::Text waitText;
        waitText.setFont(font);
        waitText.setString("Waiting for players...");
        waitText.setCharacterSize(40);
        waitText.setFillColor(sf::Color::White);
        waitText.setPosition((SCREEN_WIDTH - waitText.getLocalBounds().width) / 2,
                           (SCREEN_HEIGHT - waitText.getLocalBounds().height) / 2);
        window.draw(waitText);
    }

    window.display();
}

void Renderer::renderMap(const Map& map) {
    int start_x = static_cast<int>(camera_x);
    int end_x = start_x + SCREEN_WIDTH / TILE_SIZE + 1;

    if (end_x > map.getWidth()) {
        end_x = map.getWidth();
    }

    // Render background first
    if (background_texture.getSize().x > 0) {
        sf::Sprite background(background_texture);
        float scale_x = static_cast<float>(SCREEN_WIDTH) / background_texture.getSize().x;
        float scale_y = static_cast<float>(SCREEN_HEIGHT) / background_texture.getSize().y;
        background.setScale(scale_x, scale_y);
        window.draw(background);
    }

    // Render tiles
    for (int y = 0; y < map.getHeight(); y++) {
        for (int x = start_x; x < end_x; x++) {
            char tile = map.getTile(x, y);

            sf::RectangleShape tileShape;
            tileShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            tileShape.setPosition((x - camera_x) * TILE_SIZE, y * TILE_SIZE);

            switch (tile) {
                case 'c': {
                    // Coin
                    if (coin_texture.getSize().x > 0) {
                        sf::Sprite coin(coin_texture);
                        coin.setPosition((x - camera_x) * TILE_SIZE, y * TILE_SIZE);
                        float scale = static_cast<float>(TILE_SIZE) / coin_texture.getSize().x;
                        coin.setScale(scale, scale);
                        window.draw(coin);
                    } else {
                        tileShape.setFillColor(sf::Color::Yellow);
                        window.draw(tileShape);
                    }
                    break;
                }

                case 'e': {
                    // Electric hazard
                    if (electric_texture.getSize().x > 0) {
                        sf::Sprite electric(electric_texture);
                        electric.setPosition((x - camera_x) * TILE_SIZE, y * TILE_SIZE);
                        float scale = static_cast<float>(TILE_SIZE) / electric_texture.getSize().x;
                        electric.setScale(scale, scale);
                        window.draw(electric);
                    } else {
                        tileShape.setFillColor(sf::Color::Red);
                        window.draw(tileShape);
                    }
                    break;
                }

                default:
                    // Empty space, don't draw anything
                    break;
            }
        }
    }
}

void Renderer::renderPlayers(GameState* state)
{
    auto players = state->getPlayers();
    int my_player_num = client->getPlayerNumber();

    for (auto& pair : players) {
        int player_num = pair.first;
        const PlayerState& player = pair.second;

        // Calculate screen position
        float x = (player.x - camera_x) * TILE_SIZE;
        float y = player.y * TILE_SIZE;

        // Draw player
        if (player_texture.getSize().x > 0) {
            sf::Sprite playerSprite(player_texture);
            playerSprite.setPosition(x, y);
            float scale = static_cast<float>(TILE_SIZE) / player_texture.getSize().x;
            playerSprite.setScale(scale, scale);

            // Highlight my player
            if (player_num == my_player_num) {
                playerSprite.setColor(sf::Color(255, 255, 255));
            } else {
                playerSprite.setColor(sf::Color(200, 200, 200));
            }

            window.draw(playerSprite);
        } else {
            sf::RectangleShape playerShape;
            playerShape.setSize(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            playerShape.setPosition(x, y);

            // Different color for each player
            if (player_num == my_player_num) {
                playerShape.setFillColor(sf::Color(0, 255, 0));
            } else {
                playerShape.setFillColor(sf::Color(255, 0, 0));
            }

            window.draw(playerShape);
        }

        // Draw jetpack if active
        if (player.jet_active) {
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

        // Draw player number and score
        sf::Text playerText;
        playerText.setFont(font);
        playerText.setString("P" + std::to_string(player_num + 1) + ": " + std::to_string(player.score));
        playerText.setCharacterSize(20);
        playerText.setFillColor(sf::Color::White);
        playerText.setOutlineColor(sf::Color::Black);
        playerText.setOutlineThickness(1);
        playerText.setPosition(x, y - 20);
        window.draw(playerText);
    }
}

void Renderer::renderEffects(GameState* state)
{
    auto effects = state->getEffects();

    for (const auto& effect : effects) {
        float x = (effect.x - camera_x) * TILE_SIZE;
        float y = effect.y * TILE_SIZE;

        sf::CircleShape effectShape;
        effectShape.setRadius(TILE_SIZE / 2);
        effectShape.setPosition(x, y);

        // Fade out based on lifetime
        int alpha = 255 * effect.lifetime / 20;

        if (effect.type == 'c') {
            // Coin effect
            effectShape.setFillColor(sf::Color(255, 255, 0, alpha));
        } else if (effect.type == 'e') {
            // Electric hazard effect
            effectShape.setFillColor(sf::Color(255, 0, 0, alpha));
        }

        window.draw(effectShape);
    }

    // Update effect lifetimes
    state->updateEffects();
}
