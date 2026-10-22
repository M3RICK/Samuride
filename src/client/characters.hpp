// A character's looks: sprite sheet, portrait and animations, loaded from assets/<dir>/meta.txt.
// The runners (common/runners.hpp) and the titans each have one.

#ifndef CHARACTERS_HPP
    #define CHARACTERS_HPP

#include <SFML/Graphics.hpp>
#include <string>

struct Animation {
    enum class Mode { Loop, Once, Hide };

    int row = 0;
    int frames = 0;
    float fps = 1;
    Mode mode = Mode::Loop;

    float duration() const { return frames / fps; }
    // Frame index after `seconds`, -1 once a Hide animation is over
    int frameAt(float seconds) const;
};

struct Character {
    std::string name = "?";
    sf::Texture sheet;
    sf::Texture portrait;
    sf::Texture faces;      // the HUD's living face, a grid of heads (game_hud.cpp); empty for most
    sf::Vector2i face_grid; // its columns and rows
    sf::Vector2i frame_size;
    float feet = 0;     // y of the feet inside a cell
    float body = 1;     // standing height in pixels, scaled to the player's hitbox
    sf::Color exhaust{252, 238, 10};

    Animation run, takeoff, rise, hover, fall, death, attack;
    Animation shoot;            // Dante's guns: his attack in the air, his sword being on the ground
    Animation smoke, emerge;    // Eren's transformation only

    bool load(const std::string &dir);
    sf::IntRect frameRect(const Animation &anim, int index) const;
};

#endif
