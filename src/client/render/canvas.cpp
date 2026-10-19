// The window and the drawing primitives every screen shares: neon palette, glitch text, cards, halos

#include "canvas.hpp"
#include "../../common/debug.hpp"
#include <algorithm>
#include <cstdlib>

using namespace Palette;

float randf(float min, float max)
{
    return min + (max - min) * (std::rand() / static_cast<float>(RAND_MAX));
}

sf::Color withAlpha(sf::Color color, uint8_t alpha)
{
    color.a = alpha;
    return color;
}

void loadTexture(sf::Texture &texture, const std::string &path, bool smooth)
{
    if (!texture.loadFromFile(path)) {
        DEBUG_LOG("Failed to load texture: " + path);
        return;
    }
    texture.setSmooth(smooth);
}

sf::Vector2f WorldView::toScreen(sf::Vector2f world) const
{
    return {(world.x - camera_x) * tile_size, Screen::PLAY_TOP + world.y * tile_size};
}

//=============================================================================
// Window
//=============================================================================

bool Canvas::open()
{
    setFullscreen(false);

    if (!font.openFromFile("assets/font/jetpack_font.ttf") || !fallback_font.openFromFile("assets/font/Rajdhani-Bold.ttf"))
        DEBUG_LOG("Failed to load fonts, text will not be displayed");

    for (char32_t c = '!'; c <= '~'; c++) {
        if (font.getGlyph(c, 32, false).bounds.size.x == 0)
            missing_glyphs += static_cast<char>(c);
    }
    return window.isOpen();
}

// Windowed, the 1920x1080 space shrinks to fit the screen; fullscreen takes the desktop's resolution
void Canvas::setFullscreen(bool fullscreen)
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    if (fullscreen) {
        window.create(desktop, "Samuride", sf::State::Fullscreen);
    } else {
        float fit = std::min({1.0f, desktop.size.x * 0.9f / Screen::WIDTH, desktop.size.y * 0.9f / Screen::HEIGHT});
        sf::Vector2u size(Screen::WIDTH * fit, Screen::HEIGHT * fit);
        window.create(sf::VideoMode(size), "Samuride", sf::Style::Titlebar | sf::Style::Close);
    }
    window.setFramerateLimit(60);
}

void Canvas::draw(const sf::Drawable &drawable, const sf::RenderStates &states)
{
    window.draw(drawable, states);
}

float Canvas::time() const
{
    return clock.getElapsedTime().asSeconds();
}

bool Canvas::blink() const
{
    return static_cast<int>(time() * 2) % 2 == 0;
}

// Black bars keep the 16:9 picture whole on screens of another shape
sf::View Canvas::screenView() const
{
    sf::View view(sf::FloatRect({0, 0}, {Screen::WIDTH, Screen::HEIGHT}));
    sf::Vector2f size(window.getSize());
    float scale = std::min(size.x / Screen::WIDTH, size.y / Screen::HEIGHT);
    sf::Vector2f used = sf::Vector2f(Screen::WIDTH, Screen::HEIGHT) * scale;

    view.setViewport(sf::FloatRect({(size.x - used.x) / 2 / size.x, (size.y - used.y) / 2 / size.y},
                                   {used.x / size.x, used.y / size.y}));
    return view;
}

//=============================================================================
// Text
//=============================================================================

sf::Text Canvas::makeText(const std::string &content, unsigned int size, sf::Color color,
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

void Canvas::text(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos, sf::Vector2f anchor)
{
    window.draw(makeText(content, size, color, pos, anchor));
}

// Chromatic split, with the odd jolt
void Canvas::glitchText(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos)
{
    float offset = size / 30.f * (std::rand() % 25 == 0 ? 4 : 1);
    sf::Text split = makeText(content, size, color, pos);

    split.setOutlineThickness(0);
    split.setFillColor(withAlpha(NEON_CYAN, 170));
    split.setPosition(pos + sf::Vector2f(-offset, 0));
    window.draw(split);

    split.setFillColor(withAlpha(NEON_RED, 170));
    split.setPosition(pos + sf::Vector2f(offset, 0));
    window.draw(split);

    text(content, size, color, pos);
}

//=============================================================================
// Shapes & Images
//=============================================================================

void Canvas::overlay(uint8_t alpha)
{
    sf::RectangleShape shade({Screen::WIDTH, Screen::HEIGHT});

    shade.setFillColor(sf::Color(0, 0, 0, alpha));
    window.draw(shade);
}

// Dark panel used by the selection screens, outlined in yellow when selected
void Canvas::card(sf::FloatRect box, bool selected)
{
    sf::RectangleShape panel(box.size);

    panel.setPosition(box.position);
    panel.setFillColor(sf::Color(10, 8, 20, 235));
    panel.setOutlineColor(selected ? NEON_YELLOW : withAlpha(NEON_CYAN, 90));
    panel.setOutlineThickness(selected ? 5 : 2);
    window.draw(panel);
}

// Additive glow disc
void Canvas::halo(sf::Vector2f center, float radius, sf::Color color)
{
    sf::CircleShape glow(radius);

    glow.setOrigin({radius, radius});
    glow.setPosition(center);
    glow.setFillColor(color);
    window.draw(glow, sf::BlendAdd);
}

// Dark disc with a neon ring: keeps a pickup or an enemy readable on any background
void Canvas::backing(sf::Vector2f center, float radius, sf::Color ring)
{
    sf::CircleShape disc(radius);

    disc.setOrigin({radius, radius});
    disc.setPosition(center);
    disc.setFillColor(sf::Color(8, 6, 16, 170));
    disc.setOutlineColor(ring);
    disc.setOutlineThickness(3);
    window.draw(disc);
}

// Outlined hexagon, turned by `angle` degrees: the shield's mark
void Canvas::hexagon(sf::Vector2f center, float radius, sf::Color color, float angle)
{
    sf::CircleShape shape(radius, 6);

    shape.setOrigin({radius, radius});
    shape.setPosition(center);
    shape.setRotation(sf::degrees(angle));
    shape.setFillColor(withAlpha(color, 50));
    shape.setOutlineColor(color);
    shape.setOutlineThickness(radius * 0.18f);
    window.draw(shape);
}

// A texture scaled to `height`, placed by its anchor (0..1 on each axis, like makeText)
void Canvas::image(const sf::Texture &texture, sf::Vector2f pos, float height, sf::Vector2f anchor, sf::Color color)
{
    sf::Vector2f size(texture.getSize());

    if (size.y == 0)
        return;

    sf::Sprite sprite(texture);
    float scale = height / size.y;

    sprite.setOrigin(size.componentWiseMul(anchor));
    sprite.setScale({scale, scale});
    sprite.setPosition(pos);
    sprite.setColor(color);
    window.draw(sprite);
}

// One frame of a horizontal strip of `frames` frames, centred and scaled to `width`
void Canvas::stripFrame(const sf::Texture &texture, int frames, int frame, sf::Vector2f center, float width)
{
    sf::Vector2i frame_size(texture.getSize().x / frames, texture.getSize().y);

    if (frame_size.x == 0)
        return;

    sf::Sprite sprite(texture);
    float scale = width / frame_size.x;

    sprite.setTextureRect(sf::IntRect({frame * frame_size.x, 0}, frame_size));
    sprite.setOrigin(sf::Vector2f(frame_size) / 2.f);
    sprite.setScale({scale, scale});
    sprite.setPosition(center);
    window.draw(sprite);
}
