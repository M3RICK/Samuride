// Everything drawn on screen: owns the window and the assets, hands each frame to the menu or the run

#ifndef RENDERER_HPP
    #define RENDERER_HPP

#include <string>
#include "canvas.hpp"
#include "sprites.hpp"
#include "backdrops.hpp"
#include "menu_screens.hpp"
#include "game_screen.hpp"
#include "../audio.hpp"

class Renderer {
private:
    Canvas canvas;
    Sprites sprites;
    Backdrops backdrops;
    Audio audio;
    MenuScreens menu_screens;
    GameScreen game_screen;

    std::string lobby_info;
    sf::Clock frame_clock;
    sf::VertexArray scanlines;

    void beginFrame(bool in_game);
    void endFrame();

public:
    Renderer();

    bool initialize();
    void render(GameState &state, bool connected);
    void renderMenu(const MenuState &menu);
    void setLobbyInfo(const std::string &info) { lobby_info = info; }
    sf::RenderWindow &getWindow() { return canvas.target(); }
    void setFullscreen(bool fullscreen) { canvas.setFullscreen(fullscreen); }
    Audio &getAudio() { return audio; }
};

#endif
