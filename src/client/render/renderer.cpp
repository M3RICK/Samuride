// Everything drawn on screen: owns the window and the assets, hands each frame to the menu or the run

#include "renderer.hpp"

Renderer::Renderer()
    : menu_screens(canvas, sprites, backdrops), game_screen(canvas, sprites, backdrops, audio),
      scanlines(sf::PrimitiveType::Lines) {
}

bool Renderer::initialize()
{
    if (!canvas.open())
        return false;

    sprites.load();
    backdrops.load();

    for (float y = 0; y < Screen::HEIGHT; y += 4) {
        scanlines.append({{0, y}, sf::Color(0, 0, 0, 45)});
        scanlines.append({{Screen::WIDTH, y}, sf::Color(0, 0, 0, 45)});
    }
    return true;
}

void Renderer::render(GameState &state, bool connected)
{
    float dt = frame_clock.restart().asSeconds();
    Phase phase = state.getPhase();

    beginFrame(phase != Phase::Waiting);
    if (phase == Phase::Waiting)
        menu_screens.drawLobby(lobby_info);
    game_screen.draw(state, phase, connected, dt);
    endFrame();
}

void Renderer::renderMenu(const MenuState &menu)
{
    // The time spent in the menu doesn't count as a frame of the run
    frame_clock.restart();
    beginFrame(false);
    menu_screens.draw(menu);
    endFrame();
}

void Renderer::beginFrame(bool in_game)
{
    sf::RenderWindow &window = canvas.target();

    audio.playMusic(in_game);
    if (!in_game)
        audio.setJetpack(false);

    window.setView(canvas.screenView());
    window.clear(sf::Color::Black);
}

void Renderer::endFrame()
{
    sf::RenderWindow &window = canvas.target();

    window.setView(canvas.screenView());
    window.draw(scanlines);
    window.display();
}
