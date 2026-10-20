// Player settings and best run, saved in the data directory

#ifndef SETTINGS_HPP
    #define SETTINGS_HPP

#include <SFML/Window/Keyboard.hpp>
#include <string>

// Player preferences, kept in the platform's data directory (see Platform::dataDirectory)
struct Settings {
    int music_volume = 50;      // percent
    int sfx_volume = 70;        // percent
    sf::Keyboard::Key jetpack_key = sf::Keyboard::Key::Space;
    sf::Keyboard::Key attack_key = sf::Keyboard::Key::F;
    sf::Keyboard::Key ultimate_key = sf::Keyboard::Key::E;
    bool fullscreen = false;

    static Settings load();
    void save() const;
};

// Readable key name for the options screen, like "SPACE" or "F"
std::string keyName(sf::Keyboard::Key key);

// Best solo distance, kept next to the settings
int loadBestDistance();
void saveBestDistance(int distance);

#endif
