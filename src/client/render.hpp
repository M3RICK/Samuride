#ifndef RENDER_HPP
#define RENDER_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include "../common/map.hpp"

class Client;
class GameState;

class Renderer {
private:
    Client* client;
    sf::RenderWindow window;

    // Game assets
    sf::Texture background_texture;
    sf::Texture player_texture;
    sf::Texture jetpack_texture;
    sf::Texture coin_texture;
    sf::Texture electric_texture;
    sf::Font font;

    // Camera position
    float camera_x;

    // Render parameters
    static constexpr int TILE_SIZE = 64; // 186 on voit johny au complet mais bon
    static constexpr int SCREEN_WIDTH = 1920;
    static constexpr int SCREEN_HEIGHT = 1080;

    // Helper methods
    void renderMap(const Map& map);
    void renderPlayers(GameState* state);
    void renderEffects(GameState* state);
    void renderHUD(GameState* state);
    void renderGameOver(GameState* state);

    sf::Clock animation_clock; // Tracks elapsed time for animations
    static constexpr float ANIMATION_FRAME_DURATION = 0.1f; // 10 frames per second

    int getCurrentAnimationFrame(bool jet_active) const {
        float elapsed = animation_clock.getElapsedTime().asSeconds();
        return static_cast<int>(elapsed / ANIMATION_FRAME_DURATION) % 4; // 4 frames per animation
    }

public:
    Renderer(Client* client);
    ~Renderer();

    bool initialize();
    void render();
    sf::RenderWindow& getWindow() { return window; }
};

#endif // RENDER_HPP
