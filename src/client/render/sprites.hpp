// Every picture of the game, loaded once and shared by the menu and the run

#ifndef SPRITES_HPP
    #define SPRITES_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include "../characters.hpp"

struct Sprites {
    static constexpr int DRONE_FRAMES = 4;

    sf::Texture poster;
    sf::Texture coin;
    sf::Texture zapper;
    sf::Texture drone;          // DRONE_FRAMES frames side by side
    sf::Texture titan_face;     // on the power-up orb
    sf::Texture hack_target;    // Lucy's glitch ring, on what her hack will hit

    std::vector<Character> characters;  // one per runner, in RUNNERS order (common/runners.hpp)
    std::vector<Character> titans;      // one per TitanType
    Character shift;            // Eren's transformation: smoke and emerging
    Character eren_titan;       // Eren's titan running, sharper than the transformation sheet's

    void load();
    // Unknown ids fall back to the first character
    const Character &character(int id) const;
};

#endif
