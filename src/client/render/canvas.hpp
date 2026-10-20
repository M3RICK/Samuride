// The window and the drawing primitives every screen shares: neon palette, glitch text, cards, halos

#ifndef CANVAS_HPP
    #define CANVAS_HPP

#include <SFML/Graphics.hpp>
#include <string>

// Everything is drawn in a fixed 1920x1080 space, the window just scales it to fit the screen
namespace Screen {
    constexpr float WIDTH = 1920;
    constexpr float HEIGHT = 1080;
    constexpr sf::Vector2f CENTER{WIDTH / 2, HEIGHT / 2};

    // The map is stretched between these two screen lines
    constexpr float PLAY_TOP = 40;
    constexpr float PLAY_BOTTOM = 1000;
}

// Night City palette
namespace Palette {
    constexpr sf::Color NEON_YELLOW{252, 238, 10};
    constexpr sf::Color NEON_CYAN{0, 240, 255};
    constexpr sf::Color NEON_RED{255, 0, 60};
    constexpr sf::Color NEON_PINK{255, 40, 200};
    constexpr sf::Color NEON_ORANGE{255, 140, 0};
    constexpr sf::Color NEON_GREEN{60, 255, 120};
    constexpr sf::Color NEON_PURPLE{190, 70, 255};
    constexpr sf::Color NEON_MAGENTA{255, 0, 255};
    // Power-ups keep their own colour wherever they show: not the zappers' cyan
    constexpr sf::Color SHIELD_COLOR = NEON_GREEN;
    constexpr sf::Color SANDEVISTAN_COLOR = NEON_MAGENTA;
    constexpr sf::Color BERSERK_COLOR{255, 200, 60};   // gold
    constexpr sf::Color DIM_WHITE{200, 200, 200};
    constexpr sf::Color HINT_GREY{150, 150, 150};
    constexpr sf::Color FADED{255, 255, 255, 130};   // unselected cards
}

// A power-up tile's colour, wherever it shows
sf::Color powerUpColor(char tile);
const char *powerUpName(char tile);

// Uniform random float in [min, max]
float randf(float min, float max);

sf::Color withAlpha(sf::Color color, uint8_t alpha);

void loadTexture(sf::Texture &texture, const std::string &path, bool smooth = true);

// World tiles to screen pixels, for a run or the menu's scrolling backdrop
struct WorldView {
    float camera_x = 0;
    float tile_size = 96;

    sf::Vector2f toScreen(sf::Vector2f world) const;
};

class Canvas {
private:
    sf::RenderWindow window;
    sf::Font font;
    // The jetpack font has no punctuation: text that needs some falls back to this one
    sf::Font fallback_font;
    std::string missing_glyphs;
    sf::Clock clock;

public:
    // Opens the window and loads the fonts
    bool open();
    // Recreates the window, covering the whole screen or not
    void setFullscreen(bool fullscreen);

    sf::RenderWindow &target() { return window; }
    void draw(const sf::Drawable &drawable, const sf::RenderStates &states = sf::RenderStates::Default);

    // Seconds since start, for animations; blink() flips twice a second
    float time() const;
    bool blink() const;

    sf::View screenView() const;

    sf::Text makeText(const std::string &content, unsigned int size, sf::Color color,
                      sf::Vector2f pos, sf::Vector2f anchor = {0.5f, 0.5f});
    void text(const std::string &content, unsigned int size, sf::Color color,
              sf::Vector2f pos, sf::Vector2f anchor = {0.5f, 0.5f});
    void glitchText(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos);

    void overlay(uint8_t alpha);
    void card(sf::FloatRect box, bool selected);
    void halo(sf::Vector2f center, float radius, sf::Color color);
    void backing(sf::Vector2f center, float radius, sf::Color ring);
    void hexagon(sf::Vector2f center, float radius, sf::Color color, float angle);
    void image(const sf::Texture &texture, sf::Vector2f pos, float height, sf::Vector2f anchor, sf::Color color);
    void stripFrame(const sf::Texture &texture, int frames, int frame, sf::Vector2f center, float width);
};

#endif
