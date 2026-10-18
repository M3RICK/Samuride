// Map tiles: coins, zappers, CYBERPSYCHO drones and Attack Titan orbs

#ifndef TILES_HPP
    #define TILES_HPP

#include <SFML/Graphics.hpp>
#include <set>
#include <utility>

class Canvas;
struct Sprites;
struct Run;

class Tiles {
private:
    Canvas &canvas;
    const Sprites &sprites;
    const Run &run;

    void drawCoin(int x, int y, float time);
    void drawZapper(int x, int y, float time);
    void drawBeam(sf::Vector2f from, sf::Vector2f to);
    void drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color);
    void drawDrone(int x, int y, float time);
    void drawShiftOrb(int x, int y, float time);
    void drawPowerUp(int x, int y, char tile, float time);

public:
    Tiles(Canvas &canvas, const Sprites &sprites, const Run &run);

    // The tiles on screen, minus what you collected and the drones shot down
    void draw(const std::set<std::pair<int, int>> &collected, const std::set<std::pair<int, int>> &destroyed);
    // Fits a texture inside the tile at world position, centered
    void drawSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color);
};

#endif
