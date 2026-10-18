/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef SETTINGS_HPP
    #define SETTINGS_HPP

#include <SFML/Window/Keyboard.hpp>
#include <string>

// Player preferences, kept in ~/.local/share/samuride/settings
struct Settings {
    int music_volume = 50;      // percent
    int sfx_volume = 70;        // percent
    sf::Keyboard::Key jetpack_key = sf::Keyboard::Key::Space;
    sf::Keyboard::Key attack_key = sf::Keyboard::Key::F;

    static Settings load();
    void save() const;
};

// Readable key name for the options screen, like "SPACE" or "F"
std::string keyName(sf::Keyboard::Key key);

// Best solo distance, kept next to the settings
int loadBestDistance();
void saveBestDistance(int distance);

#endif
