// Menu screens: main menu, options, character, mode and district select, lobby

#ifndef MENU_SCREENS_HPP
    #define MENU_SCREENS_HPP

#include <string>
#include "canvas.hpp"
#include "backdrops.hpp"

struct MenuState;
struct Sprites;
struct Character;

class MenuScreens {
private:
    Canvas &canvas;
    const Sprites &sprites;
    const Backdrops &backdrops;
    WorldView view;     // the scenery scrolling behind the menu

    void drawBackground(const MenuState &menu);
    void drawPoster();
    void drawTagline();

    void drawMain(const MenuState &menu);
    void drawMenuItem(int index, bool selected);
    void drawJoinField(const MenuState &menu);

    void drawCharacterSelect(const MenuState &menu);
    void drawCharacterCard(int index, bool selected, float time);
    void drawCharacterPreview(const Character &character, sf::FloatRect box, bool selected, float time);

    void drawOptions(const MenuState &menu);
    void drawOptionRow(int index, const MenuState &menu);
    void drawVolumeBar(int volume, sf::Vector2f right, bool selected);

    void drawModeSelect(const MenuState &menu);
    void drawModeCard(int index, bool selected);

    void drawDistrictSelect(const MenuState &menu);
    void drawDistrictCard(int index, bool selected);
    void drawThumbnail(const Backdrops::Segment &segment, sf::FloatRect dest, float from, float to);

public:
    MenuScreens(Canvas &canvas, const Sprites &sprites, const Backdrops &backdrops);

    void draw(const MenuState &menu);
    // Waiting for the second player, `info` says where they can join
    void drawLobby(const std::string &info);
};

#endif
