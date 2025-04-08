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
    static constexpr int TILE_SIZE = 40;
    static constexpr int SCREEN_WIDTH = 800;
    static constexpr int SCREEN_HEIGHT = 600;

    // Helper methods
    void renderMap(const Map& map);
    void renderPlayers(GameState* state);
    void renderEffects(GameState* state);
    void renderHUD(GameState* state);
    void renderGameOver(GameState* state);

public:
    Renderer(Client* client);
    ~Renderer();

    bool initialize();
    void render();
    sf::RenderWindow& getWindow() { return window; }
};

#endif // RENDER_HPP
