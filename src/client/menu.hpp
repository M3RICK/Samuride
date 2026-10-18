/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef MENU_HPP
    #define MENU_HPP

#include <SFML/Graphics.hpp>
#include <string>

class Renderer;

struct MenuChoice {
    enum class Mode { Solo, Host, Join, Quit };

    Mode mode;
    std::string host;
    int port;
    int character;
    int district;       // -1 when joining: the host picks
};

struct MenuState {
    enum class Screen { Main, Join, Character, District };

    Screen screen = Screen::Main;
    int selected = 0;
    int character = 0;
    int district = 0;
    std::string address = "127.0.0.1:4242";
    std::string error;
    int best_distance = 0;
};

// Shared by the menu (mouse hit tests) and the renderer (drawing)
namespace MenuLayout {
    constexpr const char *ITEMS[] = {"SOLO RUN", "HOST A RUN", "JOIN A RUN", "JACK OUT"};
    constexpr int ITEM_COUNT = 4;
    constexpr float X = 1380;
    constexpr float Y = 470;
    constexpr float SPACING = 95;
    constexpr float WIDTH = 620;
    constexpr float HEIGHT = 80;

    // Character cards, centred on the screen
    constexpr float CARD_Y = 560;
    constexpr float CARD_WIDTH = 300;
    constexpr float CARD_HEIGHT = 520;
    constexpr float CARD_SPACING = 350;

    inline float cardX(int index, int count) { return 960 + (index - (count - 1) / 2.0f) * CARD_SPACING; }

    // Districts: the three backdrops, then the corridor and Worlds Collide taking turns
    constexpr int DISTRICT_COUNT = 4;
    constexpr int DISTRICT_MIX = 3;
    constexpr float DISTRICT_Y = 540;
    constexpr float DISTRICT_WIDTH = 400;
    constexpr float DISTRICT_HEIGHT = 420;

    inline float districtX(int index) { return 960 + (index - 1.5f) * 440.0f; }
}

class Menu {
private:
    sf::RenderWindow &window;
    MenuState state;
    MenuChoice pending;

    bool handleEvent(const sf::Event &event, MenuChoice &choice);
    bool activate(MenuChoice &choice);
    bool parseAddress();
    bool handleCharacterEvent(const sf::Event &event, MenuChoice &choice);
    bool handleDistrictEvent(const sf::Event &event, MenuChoice &choice);

public:
    explicit Menu(sf::RenderWindow &window);

    // Shows the menu until the player picks something
    MenuChoice run(Renderer &renderer);
    void setError(const std::string &error) { state.error = error; }
};

// Best solo distance, kept in ~/.local/share/samuride
int loadBestDistance();
void saveBestDistance(int distance);

#endif
