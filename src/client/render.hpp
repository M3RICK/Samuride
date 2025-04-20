/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef RENDER_HPP
#define RENDER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <map>
#include "../common/map.hpp"
#include "state.hpp"

class Client;

class Renderer {
private:
    Client *client;
    sf::RenderWindow window;

    // Game assets
    sf::Texture background_texture;
    sf::Texture waiting_screen_texture;
    sf::Texture johny_texture;
    sf::Texture david_texture;
    sf::Texture jetpack_texture;
    sf::Texture coin_texture;
    sf::Texture electric_texture;
    sf::Font font;

    // Audio
    sf::Music waiting_music;
    sf::Music background_music;

    // Camera position
    float camera_x;


    //===========================================================================
    // Background Scroll
    //===========================================================================

    float background_offset;
    float scroll_speed;

    //===========================================================================
    // Render parameters
    //===========================================================================
    static constexpr int TILE_SIZE = 64;
    static constexpr int SCREEN_WIDTH = 1920;
    static constexpr int SCREEN_HEIGHT = 1080;

    // Animation
    sf::Clock animation_clock;
    static constexpr float ANIMATION_FRAME_DURATION = 0.1f;

    //===========================================================================
    // Initialization Helpers
    //===========================================================================
    bool createWindow();
    void loadAssets();
    void loadTexture(sf::Texture& texture, const std::string &path, const std::string &name);
    void loadFont();
    void loadAudio();

    //===========================================================================
    // Main Render Loop Helpers
    //===========================================================================
    void updateCamera(GameState *state);
    void renderGameScreen(GameState *state);
    void renderWaitingScreen();

    //===========================================================================
    // Map Rendering
    //===========================================================================
    void renderMap(const Map &map);
    void renderBackground();
    void renderMapTiles(const Map &map);
    void renderTile(char tile, int x, int y);
    void renderCoin(float x, float y);
    void renderElectric(float x, float y);

    //===========================================================================
    // Player Rendering
    //===========================================================================
    void renderPlayers(GameState *state);
    void renderPlayer(const PlayerState &player, int player_num, float x, float y, int my_player_num);
    void renderPlayerSprite(const PlayerState &player, int player_num, float x, float y, int my_player_num);
    void renderPlayerFallback(int player_num, float x, float y, int my_player_num);
    void renderJetpack(float x, float y);

    //===========================================================================
    // Effects Rendering
    //===========================================================================
    void renderEffects(GameState *state);
    void renderEffect(const GameState::CollisionEffect& effect, float x, float y);

    //===========================================================================
    // COUNTDOWN
    //===========================================================================

    bool show_countdown;
    int countdown_value;
    std::chrono::steady_clock::time_point countdown_time;
    void setCountdown(int value);

    //===========================================================================
    // HUD and UI Rendering
    //===========================================================================
    void renderHUD(GameState *state);
    void renderGameOver(GameState *state);
    void renderGameOverBackground();
    void renderGameOverTitle();
    void renderWinnerText(GameState *state);
    void renderFinalScores(GameState *state);
    void renderExitInstructions();

    //===========================================================================
    // Helper Functions
    //===========================================================================
    int getCurrentAnimationFrame(bool jet_active) const;
    void setupText(sf::Text& text, const std::string& content, unsigned int size, const sf::Color& color);
    void centerText(sf::Text& text, float x, float y);

public:
    Renderer(Client* client);
    ~Renderer();

    bool initialize();
    void render();
    sf::RenderWindow& getWindow() { return window; }
};

#endif // RENDER_HPP
