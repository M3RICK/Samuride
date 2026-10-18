/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "menu.hpp"
#include "render.hpp"
#include "characters.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>

Menu::Menu(sf::RenderWindow &window) : window(window), pending{MenuChoice::Mode::Quit, "", 0, 0, -1} {
}

MenuChoice Menu::run(Renderer &renderer)
{
    MenuChoice choice = {MenuChoice::Mode::Quit, "", 0, 0, -1};

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

// Returns true once a choice is made
bool Menu::handleEvent(const sf::Event &event, MenuChoice &choice)
{
    if (event.is<sf::Event::Closed>())
        return true;

    if (state.screen == MenuState::Screen::Character)
        return handleCharacterEvent(event, choice);
    if (state.screen == MenuState::Screen::District)
        return handleDistrictEvent(event, choice);

    if (state.screen == MenuState::Screen::Join) {
        if (const auto *text = event.getIf<sf::Event::TextEntered>()) {
            char32_t c = text->unicode;
            bool allowed = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                           c == '.' || c == ':' || c == '-';
            if (allowed && state.address.size() < 40)
                state.address += static_cast<char>(c);
        } else if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Backspace && !state.address.empty())
                state.address.pop_back();
            else if (key->code == sf::Keyboard::Key::Escape)
                state.screen = MenuState::Screen::Main;
            else if (key->code == sf::Keyboard::Key::Enter)
                parseAddress();
        }
        return false;
    }

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
            state.selected = (state.selected + MenuLayout::ITEM_COUNT - 1) % MenuLayout::ITEM_COUNT;
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
            state.selected = (state.selected + 1) % MenuLayout::ITEM_COUNT;
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
            return activate(choice);
        return false;
    }

    // Mouse: hover selects, click activates
    sf::Vector2i pixel;
    if (const auto *move = event.getIf<sf::Event::MouseMoved>())
        pixel = move->position;
    else if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>())
        pixel = click->position;
    else
        return false;

    sf::Vector2f pos = window.mapPixelToCoords(pixel);
    for (int i = 0; i < MenuLayout::ITEM_COUNT; i++) {
        float y = MenuLayout::Y + i * MenuLayout::SPACING;
        if (std::abs(pos.x - MenuLayout::X) < MenuLayout::WIDTH / 2 && std::abs(pos.y - y) < MenuLayout::HEIGHT / 2) {
            state.selected = i;
            if (event.is<sf::Event::MouseButtonPressed>())
                return activate(choice);
        }
    }
    return false;
}

bool Menu::activate(MenuChoice &choice)
{
    static constexpr MenuChoice::Mode modes[] = {
        MenuChoice::Mode::Solo, MenuChoice::Mode::Host, MenuChoice::Mode::Join, MenuChoice::Mode::Quit
    };

    state.error.clear();
    if (modes[state.selected] == MenuChoice::Mode::Join) {
        state.screen = MenuState::Screen::Join;
        return false;
    }
    choice.mode = modes[state.selected];
    if (choice.mode == MenuChoice::Mode::Quit)
        return true;

    // Every run goes through the character select
    pending = {choice.mode, "", 0, 0, -1};
    state.screen = MenuState::Screen::Character;
    return false;
}

bool Menu::handleCharacterEvent(const sf::Event &event, MenuChoice &choice)
{
    int count = CHARACTER_DIRS.size();
    bool confirm = false;

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
            state.character = (state.character + count - 1) % count;
        else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
            state.character = (state.character + 1) % count;
        else if (key->code == sf::Keyboard::Key::Escape)
            state.screen = MenuState::Screen::Main;
        confirm = key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space;
    }

    sf::Vector2i pixel;
    if (const auto *move = event.getIf<sf::Event::MouseMoved>())
        pixel = move->position;
    else if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>())
        pixel = click->position;

    if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>()) {
        sf::Vector2f pos = window.mapPixelToCoords(pixel);
        for (int i = 0; i < count; i++) {
            if (std::abs(pos.x - MenuLayout::cardX(i, count)) < MenuLayout::CARD_WIDTH / 2 &&
                std::abs(pos.y - MenuLayout::CARD_Y) < MenuLayout::CARD_HEIGHT / 2) {
                state.character = i;
                confirm = event.is<sf::Event::MouseButtonPressed>();
            }
        }
    }

    if (!confirm)
        return false;
    pending.character = state.character;

    // Whoever joins plays in the host's district
    if (pending.mode == MenuChoice::Mode::Join) {
        choice = pending;
        state.screen = MenuState::Screen::Main;
        return true;
    }
    state.screen = MenuState::Screen::District;
    return false;
}

bool Menu::handleDistrictEvent(const sf::Event &event, MenuChoice &choice)
{
    int count = MenuLayout::DISTRICT_COUNT;
    bool confirm = false;

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
            state.district = (state.district + count - 1) % count;
        else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
            state.district = (state.district + 1) % count;
        else if (key->code == sf::Keyboard::Key::Escape)
            state.screen = MenuState::Screen::Character;
        confirm = key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space;
    }

    sf::Vector2i pixel;
    if (const auto *move = event.getIf<sf::Event::MouseMoved>())
        pixel = move->position;
    else if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>())
        pixel = click->position;

    if (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>()) {
        sf::Vector2f pos = window.mapPixelToCoords(pixel);
        for (int i = 0; i < count; i++) {
            if (std::abs(pos.x - MenuLayout::districtX(i)) < MenuLayout::DISTRICT_WIDTH / 2 &&
                std::abs(pos.y - MenuLayout::DISTRICT_Y) < MenuLayout::DISTRICT_HEIGHT / 2) {
                state.district = i;
                confirm = event.is<sf::Event::MouseButtonPressed>();
            }
        }
    }

    if (!confirm)
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

static std::filesystem::path bestDistancePath()
{
    const char *home = std::getenv("HOME");

    return std::filesystem::path(home ? home : ".") / ".local/share/samuride/best_run";
}

int loadBestDistance()
{
    std::ifstream file(bestDistancePath());
    int distance = 0;

    file >> distance;
    return distance;
}

void saveBestDistance(int distance)
{
    std::error_code ec;
    std::filesystem::create_directories(bestDistancePath().parent_path(), ec);
    std::ofstream(bestDistancePath()) << distance << std::endl;
}
