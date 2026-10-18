/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include "menu.hpp"
#include <algorithm>
#include <cmath>
#include <format>

//=============================================================================
// Menu
//=============================================================================

void Renderer::renderMenu(const MenuState &menu)
{
    frame_clock.restart();
    beginFrame(false);

    renderMenuBackground(menu);
    drawOverlay(150);

    switch (menu.screen) {
        case MenuState::Screen::Character:
            renderCharacterSelect(menu);
            break;
        case MenuState::Screen::District:
            renderDistrictSelect(menu);
            break;
        default:
            renderMainMenu(menu);
            break;
    }

    endFrame();
}

// The scenery keeps scrolling behind the menu: the corridor, or the district being picked
void Renderer::renderMenuBackground(const MenuState &menu)
{
    float time = animation_clock.getElapsedTime().asSeconds();

    tile_size = (PLAY_BOTTOM - PLAY_TOP) / 10;
    camera_x = time * 3;

    if (menu.screen != MenuState::Screen::District) {
        renderBackdrop(backdrops[0], 1);
    } else if (menu.district == MenuLayout::DISTRICT_MIX) {
        renderBackdrop(backdrops[0], 1);
        renderBackdrop(backdrops[1], 0.5f + 0.5f * std::sin(time * 0.8f));
    } else {
        renderBackdrop(backdrops[menu.district], 1);
    }
}

void Renderer::renderTagline()
{
    drawGlitchText("Wake the fuck up, Samurai.", 46, NEON_RED, {MenuLayout::X, 220});
    window.draw(makeText("We have a city to burn.", 32, NEON_CYAN, {MenuLayout::X, 290}));
}

// Main and Join screens: poster on the left, items or address field on the right
void Renderer::renderMainMenu(const MenuState &menu)
{
    float x = MenuLayout::X;

    renderPoster();
    renderTagline();

    if (menu.screen == MenuState::Screen::Main) {
        for (int i = 0; i < MenuLayout::ITEM_COUNT; i++)
            renderMenuItem(i, i == menu.selected);
    } else {
        renderJoinField(menu);
    }

    if (!menu.error.empty())
        window.draw(makeText(menu.error, 30, NEON_RED, {x, MenuLayout::Y + 4 * MenuLayout::SPACING}));
    if (menu.best_distance > 0)
        window.draw(makeText(std::format("BEST RUN  {} M", menu.best_distance), 32, NEON_YELLOW, {x, 960}));

    window.draw(makeText("[ARROWS / MOUSE] SELECT     [ENTER] CONFIRM", 22, HINT_GREY, {x, 1030}));
}

// The selected item glitches between two red markers
void Renderer::renderMenuItem(int index, bool selected)
{
    sf::Vector2f pos = MenuLayout::itemBox(index).getCenter();
    const char *label = MenuLayout::ITEMS[index];

    if (!selected) {
        window.draw(makeText(label, 46, DIM_WHITE, pos));
        return;
    }

    drawGlitchText(label, 56, NEON_YELLOW, pos);

    float marker_x = makeText(label, 56, NEON_YELLOW, pos).getGlobalBounds().size.x / 2 + 30;
    sf::RectangleShape marker({10, 46});
    marker.setOrigin({5, 23});
    marker.setFillColor(NEON_RED);
    marker.setPosition(pos - sf::Vector2f(marker_x, 0));
    window.draw(marker);
    marker.setPosition(pos + sf::Vector2f(marker_x, 0));
    window.draw(marker);
}

void Renderer::renderJoinField(const MenuState &menu)
{
    float x = MenuLayout::X;
    sf::Vector2f field_center(x, MenuLayout::Y + 110);

    window.draw(makeText("NETRUNNER ADDRESS", 40, NEON_CYAN, {x, MenuLayout::Y}));

    sf::RectangleShape field({MenuLayout::WIDTH, MenuLayout::HEIGHT});
    field.setOrigin(field.getSize() / 2.f);
    field.setPosition(field_center);
    field.setFillColor(sf::Color(0, 0, 0, 180));
    field.setOutlineColor(NEON_CYAN);
    field.setOutlineThickness(3);
    window.draw(field);

    window.draw(makeText(menu.address + (blink() ? "_" : " "), 40, NEON_YELLOW, field_center));
    window.draw(makeText("[ENTER] JACK IN     [ESC] BACK", 26, DIM_WHITE, {x, MenuLayout::Y + 220}));
}

//=============================================================================
// Character Select
//=============================================================================

void Renderer::renderCharacterSelect(const MenuState &menu)
{
    float time = animation_clock.getElapsedTime().asSeconds();

    drawGlitchText("CHOOSE YOUR RUNNER", 80, NEON_YELLOW, {SCREEN_WIDTH / 2, 150});

    for (int i = 0; i < static_cast<int>(characters.size()); i++)
        drawCharacterCard(i, i == menu.character, time);

    window.draw(makeText("[LEFT / RIGHT] CHOOSE     [ENTER] CONFIRM     [ESC] BACK", 24,
                         HINT_GREY, {SCREEN_WIDTH / 2, 1030}));
}

// Portrait, name, and the runner itself: running, then flying when selected
void Renderer::drawCharacterCard(int index, bool selected, float time)
{
    const Character &character = characters[index];
    sf::FloatRect box = MenuLayout::cardBox(index, characters.size());
    float center_x = box.getCenter().x;
    float top = box.position.y;
    sf::Color dim(255, 255, 255, 130);

    drawCard(box, selected);

    if (character.portrait.getSize().y > 0) {
        sf::Sprite portrait(character.portrait);
        float scale = 150.f / character.portrait.getSize().y;
        portrait.setScale({scale, scale});
        portrait.setOrigin({character.portrait.getSize().x / 2.f, 0});
        portrait.setPosition({center_x, top + 25});
        portrait.setColor(selected ? sf::Color::White : dim);
        window.draw(portrait);
    }

    if (selected)
        drawGlitchText(character.name, 46, NEON_YELLOW, {center_x, top + 215});
    else
        window.draw(makeText(character.name, 40, DIM_WHITE, {center_x, top + 215}));

    drawCharacterPreview(character, box, selected, time);
}

// Standing at the bottom of the card; the selected one runs, then flies
void Renderer::drawCharacterPreview(const Character &character, sf::FloatRect box, bool selected, float time)
{
    if (character.sheet.getSize().x == 0)
        return;

    bool flying = selected && static_cast<int>(time / 2.5f) % 2 == 1;
    const Animation &anim = flying ? character.hover : character.run;
    // Cells are padded for trails and blades, they may spill a little out of the card
    float scale = std::min(190.f / character.body, (box.size.x + 60) / character.frame_size.x);

    sf::Sprite sprite(character.sheet);
    sprite.setTextureRect(character.frameRect(anim, selected ? anim.frameAt(time) : 0));
    sprite.setOrigin({character.frame_size.x / 2.f, character.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition({box.getCenter().x, box.position.y + box.size.y - 30});
    sprite.setColor(selected ? sf::Color::White : sf::Color(255, 255, 255, 130));
    window.draw(sprite);
}

//=============================================================================
// District Select
//=============================================================================

void Renderer::renderDistrictSelect(const MenuState &menu)
{
    drawGlitchText("CHOOSE YOUR DISTRICT", 80, NEON_YELLOW, {SCREEN_WIDTH / 2, 110});

    for (int i = 0; i < MenuLayout::DISTRICT_COUNT; i++)
        drawDistrictCard(i, i == menu.district);

    window.draw(makeText("[ARROWS] CHOOSE     [ENTER] JACK IN     [ESC] BACK", 24,
                         HINT_GREY, {SCREEN_WIDTH / 2, 1030}));
}

void Renderer::drawDistrictCard(int index, bool selected)
{
    bool mix = index == MenuLayout::DISTRICT_MIX;
    sf::FloatRect box = MenuLayout::districtBox(index);
    float center_x = box.getCenter().x;
    float top = box.position.y;

    drawCard(box, selected);

    // One slice per picture: Worlds Collide shows its three panoramas, Both Worlds its two districts
    sf::FloatRect thumb({box.position.x + 20, top + 20}, {box.size.x - 40, 190});
    std::vector<Backdrop::Segment> slices = districtSlices(index);
    for (size_t k = 0; k < slices.size(); k++)
        drawThumbnail(slices[k], thumb, k / static_cast<float>(slices.size()), (k + 1) / static_cast<float>(slices.size()));

    std::string name = mix ? "BOTH WORLDS" : backdrops[index].name;
    std::string tagline = mix ? "Corridor and Worlds Collide, every 250 m" : backdrops[index].tagline;

    if (selected)
        drawGlitchText(name, 40, NEON_YELLOW, {center_x, top + 245});
    else
        window.draw(makeText(name, 36, DIM_WHITE, {center_x, top + 245}));
    window.draw(makeText(tagline, 24, selected ? NEON_CYAN : HINT_GREY, {center_x, top + 295}));
}

std::vector<Renderer::Backdrop::Segment> Renderer::districtSlices(int district) const
{
    if (district == MenuLayout::DISTRICT_MIX)
        return {thumbnailOf(backdrops[0], 0), thumbnailOf(backdrops[1], 0)};

    const Backdrop &backdrop = backdrops[district];
    std::vector<Backdrop::Segment> slices;
    size_t count = std::max<size_t>(1, backdrop.layers.front().segments.size());

    for (size_t k = 0; k < count; k++)
        slices.push_back(thumbnailOf(backdrop, k));
    return slices;
}

// Picture n of a backdrop's far layer, or its play band when the layer is a single image
Renderer::Backdrop::Segment Renderer::thumbnailOf(const Backdrop &backdrop, size_t index) const
{
    const std::vector<Backdrop::Segment> &segments = backdrop.layers.front().segments;
    sf::Vector2i size(backdrop.texture->getSize());

    if (!segments.empty())
        return segments[std::min(index, segments.size() - 1)];
    return {backdrop.texture, sf::IntRect({0, static_cast<int>(backdrop.ceiling * size.y)},
                                          {size.x, static_cast<int>((backdrop.floor - backdrop.ceiling) * size.y)})};
}

// The left of a picture squeezed into the from..to slice of dest
void Renderer::drawThumbnail(const Backdrop::Segment &segment, sf::FloatRect dest, float from, float to)
{
    sf::FloatRect source(segment.source);

    if (source.size.y == 0)
        return;

    float slice_w = dest.size.x * (to - from);
    float source_w = std::min(source.size.x, source.size.y * slice_w / dest.size.y);
    sf::Vector2i rect_size(source_w, source.size.y);

    sf::Sprite sprite(*segment.texture);
    sprite.setTextureRect(sf::IntRect(sf::Vector2i(source.position), rect_size));
    sprite.setScale({slice_w / source_w, dest.size.y / source.size.y});
    sprite.setPosition(dest.position + sf::Vector2f(dest.size.x * from, 0));
    window.draw(sprite);
}

//=============================================================================
// Lobby
//=============================================================================

void Renderer::renderPoster()
{
    if (waiting_screen_texture.getSize().y == 0)
        return;

    sf::Sprite poster(waiting_screen_texture);
    float scale = SCREEN_HEIGHT * 0.85f / waiting_screen_texture.getSize().y;

    poster.setScale({scale, scale});
    poster.setOrigin(sf::Vector2f(waiting_screen_texture.getSize()) / 2.f);
    poster.setPosition({SCREEN_WIDTH * 0.28f, SCREEN_HEIGHT / 2});
    window.draw(poster);
}

void Renderer::renderWaitingScreen()
{
    float x = MenuLayout::X;

    renderPoster();
    renderTagline();

    if (blink())
        window.draw(makeText("WAITING FOR ANOTHER CHOOM...", 34, NEON_YELLOW, {x, 520}));
    if (!lobby_info.empty())
        window.draw(makeText(lobby_info, 28, NEON_CYAN, {x, 600}));

    window.draw(makeText("[SPACE] JETPACK     [ESC] BACK TO MENU", 24, DIM_WHITE, {x, 1030}));
}
