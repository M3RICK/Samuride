// Menu logic: navigation, options, upgrades shop, character, mode and district select

#ifndef MENU_HPP
    #define MENU_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "settings.hpp"
#include "wallet.hpp"

class Renderer;
class Audio;

struct MenuChoice {
    enum class Mode { Solo, Host, Join, Quit };

    Mode mode;
    std::string host;
    int port;
    int character;
    int district;           // -1 when joining: the host picks
    int game_mode = -1;     // GameMode, -1 when joining
};

struct MenuState {
    enum class Screen { Main, Join, Options, Upgrades, Character, Mode, District };

    Screen screen = Screen::Main;
    int selected = 0;
    int character = 0;
    int game_mode = 0;
    int district = 0;
    int option = 0;
    int upgrade = 0;
    bool waiting_for_key = false;   // options: the next key pressed gets bound
    Settings settings;
    std::string address = "127.0.0.1:4242";
    std::string error;
    Bests bests;
    Wallet wallet;
};

// Shared by the menu (mouse hit tests) and the renderer (drawing)
namespace MenuLayout {
    constexpr const char *ITEMS[] = {"SOLO RUN", "HOST A RUN", "JOIN A RUN", "UPGRADES", "OPTIONS", "JACK OUT"};
    enum Item { ITEM_SOLO, ITEM_HOST, ITEM_JOIN, ITEM_UPGRADES, ITEM_OPTIONS, ITEM_QUIT, ITEM_COUNT };
    constexpr float X = 1380;
    constexpr float Y = 410;
    constexpr float SPACING = 80;

    // Options rows
    enum Option { OPTION_MUSIC, OPTION_SFX, OPTION_JETPACK, OPTION_ATTACK, OPTION_ULTIMATE, OPTION_FULLSCREEN, OPTION_BACK, OPTION_COUNT };
    constexpr float WIDTH = 620;
    constexpr float HEIGHT = 80;

    // Character cards, five to a row, the next row starting under Johnny
    constexpr int CARD_COLUMNS = 5;
    constexpr float CARD_Y = 445;
    constexpr float CARD_WIDTH = 280;
    constexpr float CARD_HEIGHT = 330;
    constexpr float CARD_SPACING = 305;
    constexpr float CARD_ROW_SPACING = 350;

    // Districts: the five backdrops, then the corridor and Worlds Collide taking turns. 3x2 grid.
    constexpr int DISTRICT_COUNT = 6;
    constexpr int DISTRICT_MIX = 5;
    constexpr int DISTRICT_COLUMNS = 3;
    constexpr float DISTRICT_WIDTH = 440;
    constexpr float DISTRICT_HEIGHT = 340;

    inline sf::FloatRect centeredBox(sf::Vector2f center, sf::Vector2f size)
    {
        return {center - size / 2.f, size};
    }

    // Game modes, two big cards
    constexpr int MODE_COUNT = 2;

    inline sf::FloatRect modeBox(int index)
    {
        return centeredBox({960 + (index - 0.5f) * 640, 560}, {560, 520});
    }

    inline sf::FloatRect optionBox(int index)
    {
        return centeredBox({960, 300 + index * 105.0f}, {1000, 90});
    }

    // Segment i (0-9) of the volume bar on an option row, lit up to the volume
    inline sf::FloatRect volumeSegment(int row, int i)
    {
        sf::FloatRect box = optionBox(row);
        float right = box.position.x + box.size.x - 40;
        return {{right - 120 - (10 - i) * 28, box.getCenter().y - 15}, {22, 30}};
    }

    inline sf::FloatRect itemBox(int index)
    {
        return centeredBox({X, Y + index * SPACING}, {WIDTH, HEIGHT});
    }

    inline sf::FloatRect cardBox(int index)
    {
        float x = 960 + (index % CARD_COLUMNS - (CARD_COLUMNS - 1) / 2.0f) * CARD_SPACING;
        float y = CARD_Y + (index / CARD_COLUMNS) * CARD_ROW_SPACING;
        return centeredBox({x, y}, {CARD_WIDTH, CARD_HEIGHT});
    }

    // Upgrades shop: one card each, in a row
    inline sf::FloatRect upgradeBox(int index)
    {
        return centeredBox({960 + (index - (UPGRADE_COUNT - 1) / 2.0f) * 440, 580}, {400, 480});
    }

    inline sf::FloatRect districtBox(int index)
    {
        float x = 960 + (index % DISTRICT_COLUMNS - 1) * 490.0f;
        float y = 395 + (index / DISTRICT_COLUMNS) * 375.0f;
        return centeredBox({x, y}, {DISTRICT_WIDTH, DISTRICT_HEIGHT});
    }
}

class Menu {
private:
    sf::RenderWindow &window;
    MenuState state;
    MenuChoice pending;
    Renderer *renderer = nullptr;   // set by run()
    Audio *audio = nullptr;         // the renderer's

    bool handleEvent(const sf::Event &event, MenuChoice &choice);
    bool handleMainEvent(const sf::Event &event, MenuChoice &choice);
    void handleOptionsEvent(const sf::Event &event);
    void changeVolume(int delta);
    bool pointVolume(const sf::Event &event);
    void bindKey(sf::Keyboard::Key key);
    void leaveOptions();
    void handleJoinEvent(const sf::Event &event);
    void handleUpgradesEvent(const sf::Event &event);
    bool handleCharacterEvent(const sf::Event &event, MenuChoice &choice);
    void buyCharacter();
    void handleModeEvent(const sf::Event &event);
    bool handleDistrictEvent(const sf::Event &event, MenuChoice &choice);

    bool navigate(const sf::Event &event, int &selected, int count, int columns,
                  const std::function<sf::FloatRect(int)> &box);
    int boxUnderPointer(const sf::Event &event, int count, const std::function<sf::FloatRect(int)> &box) const;
    bool activate(MenuChoice &choice);
    bool parseAddress();

public:
    Menu(sf::RenderWindow &window, const Settings &settings);

    // Shows the menu until the player picks something
    MenuChoice run(Renderer &renderer);
    void setError(const std::string &error) { state.error = error; }
    const Settings &getSettings() const { return state.settings; }
};

#endif
