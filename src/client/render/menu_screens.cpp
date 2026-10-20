// Menu screens: main menu, options, character, mode and district select, lobby

#include "menu_screens.hpp"
#include "sprites.hpp"
#include "../menu.hpp"
#include "../wallet.hpp"
#include "../touch.hpp"
#include "../../common/cyberpsycho.hpp"
#include <algorithm>
#include <cmath>
#include <format>

using namespace Palette;

MenuScreens::MenuScreens(Canvas &canvas, const Sprites &sprites, const Backdrops &backdrops)
    : canvas(canvas), sprites(sprites), backdrops(backdrops) {
    view.tile_size = (Screen::PLAY_BOTTOM - Screen::PLAY_TOP) / 10;
}

void MenuScreens::draw(const MenuState &menu)
{
    drawBackground(menu);
    canvas.overlay(150);

    switch (menu.screen) {
        case MenuState::Screen::Character:
            drawCharacterSelect(menu);
            break;
        case MenuState::Screen::Options:
            drawOptions(menu);
            break;
        case MenuState::Screen::Upgrades:
            drawUpgrades(menu);
            break;
        case MenuState::Screen::Mode:
            drawModeSelect(menu);
            break;
        case MenuState::Screen::District:
            drawDistrictSelect(menu);
            break;
        default:
            drawMain(menu);
            break;
    }
}

// The scenery keeps scrolling behind the menu: the corridor, or the district being picked
void MenuScreens::drawBackground(const MenuState &menu)
{
    float time = canvas.time();

    view.camera_x = time * 3;

    if (menu.screen != MenuState::Screen::District) {
        backdrops.draw(canvas, view, 0, 1);
    } else if (menu.district == MenuLayout::DISTRICT_MIX) {
        backdrops.draw(canvas, view, 0, 1);
        backdrops.draw(canvas, view, 1, 0.5f + 0.5f * std::sin(time * 0.8f));
    } else {
        backdrops.draw(canvas, view, menu.district, 1);
    }
}

void MenuScreens::drawPoster()
{
    if (sprites.poster.getSize().y == 0)
        return;

    sf::Sprite poster(sprites.poster);
    float scale = Screen::HEIGHT * 0.85f / sprites.poster.getSize().y;

    poster.setScale({scale, scale});
    poster.setOrigin(sf::Vector2f(sprites.poster.getSize()) / 2.f);
    poster.setPosition({Screen::WIDTH * 0.28f, Screen::HEIGHT / 2});
    canvas.draw(poster);
}

void MenuScreens::drawTagline()
{
    canvas.glitchText("Wake the fuck up, Samurai.", 46, NEON_RED, {MenuLayout::X, 220});
    canvas.text("We have a city to burn.", 32, NEON_CYAN, {MenuLayout::X, 290});
}

//=============================================================================
// Main Menu & Lobby
//=============================================================================

// Main and Join screens: poster on the left, items or address field on the right
void MenuScreens::drawMain(const MenuState &menu)
{
    float x = MenuLayout::X;

    drawPoster();
    drawTagline();

    if (menu.screen == MenuState::Screen::Main) {
        for (int i = 0; i < MenuLayout::ITEM_COUNT; i++)
            drawMenuItem(i, i == menu.selected);
    } else {
        drawJoinField(menu);
    }

    if (!menu.error.empty())
        canvas.text(menu.error, 30, NEON_RED, MenuLayout::itemBox(MenuLayout::ITEM_COUNT).getCenter());
    if (menu.best_distance > 0)
        canvas.text(std::format("BEST RUN  {} M     {} EDDIES", menu.best_distance, menu.wallet.eddies),
                    32, NEON_YELLOW, {x, 960});

    canvas.text(Touch::hint("[ARROWS / MOUSE] SELECT     [ENTER] CONFIRM", "TAP TO SELECT"), 22, HINT_GREY, {x, 1030});
}

// The selected item glitches between two red markers
void MenuScreens::drawMenuItem(int index, bool selected)
{
    sf::Vector2f pos = MenuLayout::itemBox(index).getCenter();
    const char *label = MenuLayout::ITEMS[index];

    if (!selected) {
        canvas.text(label, 46, DIM_WHITE, pos);
        return;
    }

    canvas.glitchText(label, 56, NEON_YELLOW, pos);

    float marker_x = canvas.makeText(label, 56, NEON_YELLOW, pos).getGlobalBounds().size.x / 2 + 30;
    sf::RectangleShape marker({10, 46});
    marker.setOrigin({5, 23});
    marker.setFillColor(NEON_RED);
    marker.setPosition(pos - sf::Vector2f(marker_x, 0));
    canvas.draw(marker);
    marker.setPosition(pos + sf::Vector2f(marker_x, 0));
    canvas.draw(marker);
}

void MenuScreens::drawJoinField(const MenuState &menu)
{
    float x = MenuLayout::X;
    sf::Vector2f field_center(x, MenuLayout::Y + 110);

    canvas.text("NETRUNNER ADDRESS", 40, NEON_CYAN, {x, MenuLayout::Y});

    sf::RectangleShape field({MenuLayout::WIDTH, MenuLayout::HEIGHT});
    field.setOrigin(field.getSize() / 2.f);
    field.setPosition(field_center);
    field.setFillColor(sf::Color(0, 0, 0, 180));
    field.setOutlineColor(NEON_CYAN);
    field.setOutlineThickness(3);
    canvas.draw(field);

    canvas.text(menu.address + (canvas.blink() ? "_" : " "), 40, NEON_YELLOW, field_center);
    canvas.text(Touch::hint("[ENTER] JACK IN     [ESC] BACK", "TAP TO TYPE     [ENTER] JACK IN     [BACK] BACK"), 26, DIM_WHITE, {x, MenuLayout::Y + 220});
}

void MenuScreens::drawLobby(const std::string &info)
{
    float x = MenuLayout::X;

    drawPoster();
    drawTagline();

    if (canvas.blink())
        canvas.text("WAITING FOR ANOTHER CHOOM...", 34, NEON_YELLOW, {x, 520});
    if (!info.empty())
        canvas.text(info, 28, NEON_CYAN, {x, 600});

    canvas.text(Touch::hint("[SPACE] JETPACK     [ESC] BACK TO MENU", "HOLD THE SCREEN: JETPACK     [BACK] MENU"), 24, DIM_WHITE, {x, 1030});
}

//=============================================================================
// Character Select
//=============================================================================

void MenuScreens::drawCharacterSelect(const MenuState &menu)
{
    float time = canvas.time();

    canvas.glitchText("CHOOSE YOUR RUNNER", 80, NEON_YELLOW, {Screen::WIDTH / 2, 150});
    canvas.text(std::format("{} EDDIES", menu.wallet.eddies), 36, NEON_PINK, {Screen::WIDTH / 2, 235});

    for (int i = 0; i < static_cast<int>(sprites.characters.size()); i++)
        drawCharacterCard(i, i == menu.character, menu.wallet, time);

    if (!menu.error.empty())
        canvas.text(menu.error, 30, NEON_RED, {Screen::WIDTH / 2, 975});

    canvas.text(Touch::hint("[LEFT / RIGHT] CHOOSE     [ENTER] CONFIRM     [ESC] BACK", "TAP TO CHOOSE     [BACK] BACK"), 24, HINT_GREY, {Screen::WIDTH / 2, 1030});
}

// Portrait, name, and the runner itself: running, then flying when selected.
// A locked runner is a dark silhouette with its price.
void MenuScreens::drawCharacterCard(int index, bool selected, const Wallet &wallet, float time)
{
    const Character &character = sprites.characters[index];
    sf::FloatRect box = MenuLayout::cardBox(index, sprites.characters.size());
    float center_x = box.getCenter().x;
    float top = box.position.y;
    bool locked = !wallet.isUnlocked(index);
    sf::Color tint = locked ? sf::Color(25, 20, 40) : selected ? sf::Color::White : FADED;

    canvas.card(box, selected);
    canvas.image(character.portrait, {center_x, top + 25}, 150, {0.5f, 0}, tint);

    if (selected)
        canvas.glitchText(character.name, 46, NEON_YELLOW, {center_x, top + 215});
    else
        canvas.text(character.name, 40, DIM_WHITE, {center_x, top + 215});

    drawCharacterPreview(character, box, selected && !locked, tint, time);
    if (locked)
        drawPriceTag(index, box, wallet);
}

// "LOCKED" and the price, lit up when it's the next one and you can afford it
void MenuScreens::drawPriceTag(int index, sf::FloatRect box, const Wallet &wallet)
{
    int price = RUNNERS[index].price;
    bool affordable = index == wallet.unlocked && wallet.eddies >= price;
    sf::Vector2f center = box.getCenter() + sf::Vector2f(0, 90);

    canvas.text("LOCKED", 34, DIM_WHITE, center);
    canvas.text(std::format("{} EDDIES", price), 30, affordable ? NEON_YELLOW : HINT_GREY, center + sf::Vector2f(0, 45));
    if (affordable)
        canvas.text(Touch::hint("[ENTER] BUY", "TAP TO BUY"), 24, NEON_CYAN, center + sf::Vector2f(0, 85));
}

// Standing at the bottom of the card; the selected one runs, then flies
void MenuScreens::drawCharacterPreview(const Character &character, sf::FloatRect box, bool selected, sf::Color tint,
                                       float time)
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
    sprite.setColor(tint);
    canvas.draw(sprite);
}

//=============================================================================
// Upgrades
//=============================================================================

void MenuScreens::drawUpgrades(const MenuState &menu)
{
    canvas.glitchText("UPGRADES", 80, NEON_YELLOW, {Screen::WIDTH / 2, 150});
    canvas.text(std::format("{} EDDIES", menu.wallet.eddies), 36, NEON_PINK, {Screen::WIDTH / 2, 235});

    for (int i = 0; i < UPGRADE_COUNT; i++)
        drawUpgradeCard(i, i == menu.upgrade, menu.wallet);

    if (!menu.error.empty())
        canvas.text(menu.error, 30, NEON_RED, {Screen::WIDTH / 2, 900});
    canvas.text("For every runner. Kept small: they reward playing a lot.", 26, DIM_WHITE, {Screen::WIDTH / 2, 960});
    canvas.text(Touch::hint("[LEFT / RIGHT] CHOOSE     [ENTER] BUY     [ESC] BACK", "TAP TO BUY     [BACK] BACK"), 24,
                HINT_GREY, {Screen::WIDTH / 2, 1030});
}

// Name, what a level does, the levels bought, and the next one's price
void MenuScreens::drawUpgradeCard(int index, bool selected, const Wallet &wallet)
{
    const UpgradeInfo &info = UPGRADES[index];
    int level = wallet.upgrades[index];
    bool maxed = level >= info.max_level;
    bool affordable = !maxed && wallet.eddies >= info.prices[level];
    sf::FloatRect box = MenuLayout::upgradeBox(index);
    float center_x = box.getCenter().x;
    float top = box.position.y;

    canvas.card(box, selected);
    if (selected)
        canvas.glitchText(info.name, 40, NEON_YELLOW, {center_x, top + 70});
    else
        canvas.text(info.name, 36, DIM_WHITE, {center_x, top + 70});
    canvas.text(info.effect, 24, selected ? NEON_CYAN : HINT_GREY, {center_x, top + 150});

    // One pip per level, lit when bought
    float pips_x = center_x - (info.max_level - 1) * 30;
    for (int i = 0; i < info.max_level; i++) {
        sf::RectangleShape pip({40, 40});
        pip.setOrigin({20, 20});
        pip.setPosition({pips_x + i * 60, top + 250});
        pip.setFillColor(i < level ? NEON_GREEN : sf::Color::Transparent);
        pip.setOutlineColor(i < level ? NEON_GREEN : HINT_GREY);
        pip.setOutlineThickness(3);
        canvas.draw(pip);
    }
    canvas.text(std::format("LEVEL {} / {}", level, info.max_level), 26, DIM_WHITE, {center_x, top + 310});

    if (maxed) {
        canvas.text("MAXED", 36, NEON_GREEN, {center_x, top + 390});
        return;
    }
    canvas.text(std::format("{} EDDIES", info.prices[level]), 32, affordable ? NEON_YELLOW : HINT_GREY,
                {center_x, top + 390});
    if (selected && affordable)
        canvas.text(Touch::hint("[ENTER] BUY", "TAP TO BUY"), 24, NEON_CYAN, {center_x, top + 435});
}

//=============================================================================
// Options
//=============================================================================

void MenuScreens::drawOptions(const MenuState &menu)
{
    canvas.glitchText("OPTIONS", 80, NEON_YELLOW, {Screen::WIDTH / 2, 150});

    for (int i = 0; i < MenuLayout::OPTION_COUNT; i++)
        drawOptionRow(i, menu);

    canvas.text(Touch::hint("[UP / DOWN] SELECT     [LEFT / RIGHT] OR CLICK THE BAR: VOLUME     [ENTER] CHANGE     [ESC] SAVE & BACK",
                           "TAP THE BAR: VOLUME     [BACK] SAVE & BACK"),
                24, HINT_GREY, {Screen::WIDTH / 2, 1030});
}

// Label on the left, value on the right: a volume bar or a key
void MenuScreens::drawOptionRow(int index, const MenuState &menu)
{
    static const char *labels[] = {"MUSIC VOLUME", "EFFECTS VOLUME", "JETPACK KEY", "ATTACK KEY", "ULTIMATE KEY", "FULLSCREEN",
                                   "BACK"};
    const Settings &settings = menu.settings;
    sf::FloatRect box = MenuLayout::optionBox(index);
    bool selected = index == menu.option;
    sf::Color color = selected ? NEON_YELLOW : DIM_WHITE;
    sf::Vector2f left(box.position.x + 40, box.getCenter().y);
    sf::Vector2f right(box.position.x + box.size.x - 40, box.getCenter().y);

    canvas.card(box, selected);

    if (index == MenuLayout::OPTION_BACK) {
        canvas.text(labels[index], 40, color, box.getCenter());
        return;
    }
    canvas.text(labels[index], 36, color, left, {0, 0.5f});

    if (index == MenuLayout::OPTION_MUSIC || index == MenuLayout::OPTION_SFX) {
        drawVolumeBar(index, index == MenuLayout::OPTION_MUSIC ? settings.music_volume : settings.sfx_volume, right, selected);
        return;
    }
    if (Touch::ENABLED) {
        // No keys to bind nor window to resize on a phone
        canvas.text("-", 36, HINT_GREY, right, {1, 0.5f});
        return;
    }
    if (index == MenuLayout::OPTION_FULLSCREEN) {
        canvas.text(settings.fullscreen ? "ON" : "OFF", 36, color, right, {1, 0.5f});
        return;
    }

    bool waiting = selected && menu.waiting_for_key;
    sf::Keyboard::Key key = index == MenuLayout::OPTION_JETPACK ? settings.jetpack_key :
                            index == MenuLayout::OPTION_ATTACK ? settings.attack_key : settings.ultimate_key;
    std::string value = waiting ? (canvas.blink() ? "PRESS A KEY..." : "") : keyName(key);
    canvas.text(value, 36, waiting ? NEON_CYAN : color, right, {1, 0.5f});
}

// Ten segments lit up to the volume, then the percentage
void MenuScreens::drawVolumeBar(int row, int volume, sf::Vector2f right, bool selected)
{
    for (int i = 0; i < 10; i++) {
        sf::FloatRect box = MenuLayout::volumeSegment(row, i);
        sf::RectangleShape segment(box.size);
        segment.setPosition(box.position);
        segment.setFillColor(i < volume / 10 ? NEON_CYAN : sf::Color(255, 255, 255, 40));
        canvas.draw(segment);
    }
    canvas.text(std::format("{}%", volume), 32, selected ? NEON_YELLOW : DIM_WHITE, right, {1, 0.5f});
}

//=============================================================================
// Mode Select
//=============================================================================

void MenuScreens::drawModeSelect(const MenuState &menu)
{
    canvas.glitchText("CHOOSE YOUR MODE", 80, NEON_YELLOW, {Screen::WIDTH / 2, 150});

    for (int i = 0; i < MenuLayout::MODE_COUNT; i++)
        drawModeCard(i, i == menu.game_mode);

    canvas.text(Touch::hint("[LEFT / RIGHT] CHOOSE     [ENTER] CONFIRM     [ESC] BACK", "TAP TO CHOOSE     [BACK] BACK"), 24, HINT_GREY, {Screen::WIDTH / 2, 1030});
}

void MenuScreens::drawModeCard(int index, bool selected)
{
    bool psycho = index == static_cast<int>(GameMode::Cyberpsycho);
    sf::FloatRect box = MenuLayout::modeBox(index);
    sf::Vector2f center = box.getCenter();
    std::string name = psycho ? "CYBERPSYCHO" : "CLASSIC";
    sf::Color accent = psycho ? NEON_RED : NEON_CYAN;

    canvas.card(box, selected);

    if (selected)
        canvas.glitchText(name, 64, NEON_YELLOW, center + sf::Vector2f(0, -170));
    else
        canvas.text(name, 56, DIM_WHITE, center + sf::Vector2f(0, -170));

    // What the mode is about: zappers and eddies, or drones to gun down
    if (psycho) {
        int frame = static_cast<int>(canvas.time() * 12) % Sprites::DRONE_FRAMES;
        canvas.halo(center, 110, withAlpha(NEON_RED, 50));
        canvas.stripFrame(sprites.drone, Sprites::DRONE_FRAMES, frame, center, 260);
    } else {
        canvas.image(sprites.coin, center + sf::Vector2f(-90, 0), 120, {0.5f, 0.5f}, sf::Color::White);
        canvas.image(sprites.zapper, center + sf::Vector2f(90, 0), 110, {0.5f, 0.5f}, sf::Color::White);
    }

    std::string tagline = psycho ? "Gun down Arasaka drones for bounty" : "Dodge the zappers, grab the eddies";
    std::string detail = psycho ? std::format("{}     +{} EDDIES PER DRONE", Touch::hint("[F] ATTACK", "ATTACK BUTTON"), DRONE_BOUNTY) : "The original run";
    canvas.text(tagline, 28, selected ? accent : HINT_GREY, center + sf::Vector2f(0, 150));
    canvas.text(detail, 24, DIM_WHITE, center + sf::Vector2f(0, 200));
}

//=============================================================================
// District Select
//=============================================================================

void MenuScreens::drawDistrictSelect(const MenuState &menu)
{
    canvas.glitchText("CHOOSE YOUR DISTRICT", 80, NEON_YELLOW, {Screen::WIDTH / 2, 110});
    canvas.text(std::format("{} EDDIES", menu.wallet.eddies), 30, NEON_PINK, {Screen::WIDTH / 2, 180});

    for (int i = 0; i < MenuLayout::DISTRICT_COUNT; i++)
        drawDistrictCard(i, i == menu.district, menu.wallet);

    if (!menu.error.empty())
        canvas.text(menu.error, 30, NEON_RED, {Screen::WIDTH / 2, 985});

    canvas.text(Touch::hint("[ARROWS] CHOOSE     [ENTER] JACK IN     [ESC] BACK", "TAP TO JACK IN     [BACK] BACK"), 24, HINT_GREY, {Screen::WIDTH / 2, 1030});
}

// A locked district is dimmed, with its price instead of its tagline
void MenuScreens::drawDistrictCard(int index, bool selected, const Wallet &wallet)
{
    bool locked = !wallet.hasDistrict(index);
    bool mix = index == MenuLayout::DISTRICT_MIX;
    sf::FloatRect box = MenuLayout::districtBox(index);
    float center_x = box.getCenter().x;
    float top = box.position.y;

    canvas.card(box, selected);

    sf::FloatRect thumb({box.position.x + 20, top + 20}, {box.size.x - 40, 190});
    std::vector<Backdrops::Segment> slices = backdrops.slices(index);
    for (size_t k = 0; k < slices.size(); k++)
        drawThumbnail(slices[k], thumb, k / static_cast<float>(slices.size()), (k + 1) / static_cast<float>(slices.size()));
    if (locked) {
        sf::RectangleShape shade(thumb.size);
        shade.setPosition(thumb.position);
        shade.setFillColor(sf::Color(10, 5, 20, 200));
        canvas.draw(shade);
        canvas.text("LOCKED", 40, DIM_WHITE, thumb.getCenter());
    }

    std::string name = mix ? "BOTH WORLDS" : backdrops[index].name;
    std::string tagline = mix ? "Corridor and Worlds Collide, every 250 m" : backdrops[index].tagline;

    if (selected)
        canvas.glitchText(name, 40, NEON_YELLOW, {center_x, top + 245});
    else
        canvas.text(name, 36, DIM_WHITE, {center_x, top + 245});
    if (!locked) {
        canvas.text(tagline, 24, selected ? NEON_CYAN : HINT_GREY, {center_x, top + 295});
        return;
    }

    int price = DISTRICT_PRICES[index];
    bool affordable = wallet.eddies >= price;
    std::string tag = std::format("{} EDDIES", price) + (selected && affordable ? "     [ENTER] BUY" : "");
    canvas.text(tag, 24, affordable ? NEON_YELLOW : HINT_GREY, {center_x, top + 295});
}

// The left of a picture squeezed into the from..to slice of dest
void MenuScreens::drawThumbnail(const Backdrops::Segment &segment, sf::FloatRect dest, float from, float to)
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
    canvas.draw(sprite);
}
