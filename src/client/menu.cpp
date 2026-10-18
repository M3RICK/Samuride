/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "menu.hpp"
#include "render.hpp"
#include "audio.hpp"
#include "characters.hpp"
#include <algorithm>
#include <cstdlib>

Menu::Menu(sf::RenderWindow &window, const Settings &settings)
    : window(window), pending{MenuChoice::Mode::Quit, "", 0, 0, -1} {
    state.settings = settings;
}

MenuChoice Menu::run(Renderer &renderer)
{
    MenuChoice choice = {MenuChoice::Mode::Quit, "", 0, 0, -1};

    audio = &renderer.getAudio();
    state.screen = MenuState::Screen::Main;
    state.best_distance = loadBestDistance();

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (handleEvent(*event, choice))
                return choice;
        }
        renderer.renderMenu(state);
    }
    return choice;
}

static bool isKey(const sf::Event &event, sf::Keyboard::Key code)
{
    const auto *key = event.getIf<sf::Event::KeyPressed>();

    return key && key->code == code;
}

// Arrows (or WASD) move the selection: left/right by one, up/down by a row of `columns`
static void moveSelection(int &selected, sf::Keyboard::Key key, int count, int columns)
{
    using Key = sf::Keyboard::Key;
    int step = 0;

    if (key == Key::Left || key == Key::A)
        step = -1;
    else if (key == Key::Right || key == Key::D)
        step = 1;
    else if (key == Key::Up || key == Key::W)
        step = -columns;
    else if (key == Key::Down || key == Key::S)
        step = columns;
    selected = ((selected + step) % count + count) % count;
}

// Returns true once a choice is made
bool Menu::handleEvent(const sf::Event &event, MenuChoice &choice)
{
    if (event.is<sf::Event::Closed>())
        return true;

    switch (state.screen) {
        case MenuState::Screen::Main:
            return handleMainEvent(event, choice);
        case MenuState::Screen::Join:
            handleJoinEvent(event);
            return false;
        case MenuState::Screen::Options:
            handleOptionsEvent(event);
            return false;
        case MenuState::Screen::Character:
            return handleCharacterEvent(event, choice);
        case MenuState::Screen::Mode:
            handleModeEvent(event);
            return false;
        case MenuState::Screen::District:
            return handleDistrictEvent(event, choice);
    }
    return false;
}

// Shared by every list: arrows or hovering move `selected`, Enter, Space or a click confirm it
bool Menu::navigate(const sf::Event &event, int &selected, int count, int columns,
                    const std::function<sf::FloatRect(int)> &box)
{
    int before = selected;
    bool confirmed = false;

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        moveSelection(selected, key->code, count, columns);
        confirmed = key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space;
    } else if (int hovered = boxUnderPointer(event, count, box); hovered >= 0) {
        selected = hovered;
        confirmed = event.is<sf::Event::MouseButtonPressed>();
    }

    if (confirmed)
        audio->play(Audio::Sfx::MenuConfirm);
    else if (selected != before)
        audio->play(Audio::Sfx::MenuMove);
    return confirmed;
}

// Index of the box under the mouse for a mouse event, -1 otherwise
int Menu::boxUnderPointer(const sf::Event &event, int count, const std::function<sf::FloatRect(int)> &box) const
{
    sf::Vector2i pixel;

    if (const auto *move = event.getIf<sf::Event::MouseMoved>())
        pixel = move->position;
    else if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>())
        pixel = click->position;
    else
        return -1;

    sf::Vector2f pos = window.mapPixelToCoords(pixel);
    for (int i = 0; i < count; i++) {
        if (box(i).contains(pos))
            return i;
    }
    return -1;
}

bool Menu::handleMainEvent(const sf::Event &event, MenuChoice &choice)
{
    if (navigate(event, state.selected, MenuLayout::ITEM_COUNT, 1, MenuLayout::itemBox))
        return activate(choice);
    return false;
}

void Menu::handleJoinEvent(const sf::Event &event)
{
    if (const auto *text = event.getIf<sf::Event::TextEntered>()) {
        char32_t c = text->unicode;
        bool allowed = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       c == '.' || c == ':' || c == '-';
        if (allowed && state.address.size() < 40)
            state.address += static_cast<char>(c);
    } else if (isKey(event, sf::Keyboard::Key::Backspace) && !state.address.empty()) {
        state.address.pop_back();
    } else if (isKey(event, sf::Keyboard::Key::Escape)) {
        state.screen = MenuState::Screen::Main;
    } else if (isKey(event, sf::Keyboard::Key::Enter)) {
        parseAddress();
    }
}

bool Menu::activate(MenuChoice &choice)
{
    state.error.clear();

    switch (state.selected) {
        case MenuLayout::ITEM_JOIN:
            state.screen = MenuState::Screen::Join;
            return false;
        case MenuLayout::ITEM_OPTIONS:
            state.option = 0;
            state.screen = MenuState::Screen::Options;
            return false;
        case MenuLayout::ITEM_QUIT:
            choice.mode = MenuChoice::Mode::Quit;
            return true;
        default:
            break;
    }

    // Every run goes through the character select
    choice.mode = state.selected == MenuLayout::ITEM_SOLO ? MenuChoice::Mode::Solo : MenuChoice::Mode::Host;
    pending = {choice.mode, "", 0, 0, -1};
    state.screen = MenuState::Screen::Character;
    return false;
}

//=============================================================================
// Options
//=============================================================================

void Menu::handleOptionsEvent(const sf::Event &event)
{
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    bool on_volume = state.option == MenuLayout::OPTION_MUSIC || state.option == MenuLayout::OPTION_SFX;

    // Waiting for the key to bind: Escape cancels
    if (state.waiting_for_key) {
        if (key && key->code != sf::Keyboard::Key::Escape)
            bindKey(key->code);
        else if (key)
            state.waiting_for_key = false;
        return;
    }

    if (isKey(event, sf::Keyboard::Key::Escape)) {
        leaveOptions();
        return;
    }
    // Left and right set the volume instead of moving the selection
    if (key && (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::Right)) {
        if (on_volume)
            changeVolume(key->code == sf::Keyboard::Key::Right ? 10 : -10);
        return;
    }
    if (!navigate(event, state.option, MenuLayout::OPTION_COUNT, 1, MenuLayout::optionBox))
        return;

    if (state.option == MenuLayout::OPTION_BACK)
        leaveOptions();
    else if (on_volume)
        changeVolume((state.option == MenuLayout::OPTION_MUSIC ? state.settings.music_volume
                                                                 : state.settings.sfx_volume) == 100 ? -100 : 10);
    else
        state.waiting_for_key = true;
}

// Steps of 10%, heard straight away
void Menu::changeVolume(int delta)
{
    int &volume = state.option == MenuLayout::OPTION_MUSIC ? state.settings.music_volume : state.settings.sfx_volume;

    volume = std::clamp(volume + delta, 0, 100);
    audio->setVolumes(state.settings.music_volume, state.settings.sfx_volume);
    audio->play(Audio::Sfx::MenuMove);
}

// A key already used by the other action swaps with it
void Menu::bindKey(sf::Keyboard::Key key)
{
    Settings &settings = state.settings;
    sf::Keyboard::Key &target = state.option == MenuLayout::OPTION_JETPACK ? settings.jetpack_key : settings.attack_key;
    sf::Keyboard::Key &other = state.option == MenuLayout::OPTION_JETPACK ? settings.attack_key : settings.jetpack_key;

    if (key == other)
        other = target;
    target = key;
    state.waiting_for_key = false;
    audio->play(Audio::Sfx::MenuConfirm);
}

void Menu::leaveOptions()
{
    state.settings.save();
    state.screen = MenuState::Screen::Main;
}

bool Menu::handleCharacterEvent(const sf::Event &event, MenuChoice &choice)
{
    int count = CHARACTER_DIRS.size();
    auto box = [count](int index) { return MenuLayout::cardBox(index, count); };

    if (isKey(event, sf::Keyboard::Key::Escape)) {
        state.screen = MenuState::Screen::Main;
        return false;
    }
    if (!navigate(event, state.character, count, count, box))
        return false;

    pending.character = state.character;

    // Whoever joins plays in the host's district
    if (pending.mode == MenuChoice::Mode::Join) {
        choice = pending;
        state.screen = MenuState::Screen::Main;
        return true;
    }
    state.screen = MenuState::Screen::Mode;
    return false;
}

void Menu::handleModeEvent(const sf::Event &event)
{
    if (isKey(event, sf::Keyboard::Key::Escape)) {
        state.screen = MenuState::Screen::Character;
        return;
    }
    if (!navigate(event, state.game_mode, MenuLayout::MODE_COUNT, MenuLayout::MODE_COUNT, MenuLayout::modeBox))
        return;

    pending.game_mode = state.game_mode;
    state.screen = MenuState::Screen::District;
}

bool Menu::handleDistrictEvent(const sf::Event &event, MenuChoice &choice)
{
    if (isKey(event, sf::Keyboard::Key::Escape)) {
        state.screen = MenuState::Screen::Mode;
        return false;
    }
    if (!navigate(event, state.district, MenuLayout::DISTRICT_COUNT, MenuLayout::DISTRICT_COLUMNS,
                  MenuLayout::districtBox))
        return false;

    choice = pending;
    choice.district = state.district;
    state.screen = MenuState::Screen::Main;
    return true;
}

// "host" or "host:port", 4242 by default. Moves on to the character select when valid.
bool Menu::parseAddress()
{
    size_t colon = state.address.rfind(':');
    std::string host = state.address.substr(0, colon);
    int port = colon == std::string::npos ? 4242 : std::atoi(state.address.c_str() + colon + 1);

    if (host.empty() || port <= 0 || port > 65535) {
        state.error = "INVALID ADDRESS";
        return false;
    }
    state.error.clear();
    pending = {MenuChoice::Mode::Join, host, port, 0, -1};
    state.screen = MenuState::Screen::Character;
    return false;
}
