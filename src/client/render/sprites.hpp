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
    sf::Texture fishbones;      // Jinx's shark rocket, nose to the right

    std::vector<Character> characters;  // one per runner, in RUNNERS order (common/runners.hpp)
    std::vector<Character> titans;      // one per TitanType
    Character shift;            // Eren's transformation: smoke and emerging
    Character eren_titan;       // Eren's titan running, sharper than the transformation sheet's
    std::vector<Character> mounts;  // each runner's, from assets/<dir>_mount, in place of the titan
    Character smasher;          // Adam Smasher, the boss
    Character trauma_av;        // the Trauma Team's AV, "run" is its hover
    Character trauma_medic;     // and its medic, "run" is him standing

    void load();
    // Unknown ids fall back to the first character
    const Character &character(int id) const;
    // What they run as in the titan form: their mount, or Eren's titan for those who have none
    const Character &mount(int id) const;
};

#endif
