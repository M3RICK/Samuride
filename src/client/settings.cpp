// Player settings and best run, saved in the data directory

#include "settings.hpp"
#include "../common/platform.hpp"
#include <SFML/System/String.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>

static std::filesystem::path dataPath(const std::string &file)
{
    return Platform::dataDirectory() / file;
}

static void writeFile(const std::string &file, const std::string &content)
{
    std::error_code ec;

    std::filesystem::create_directories(dataPath(file).parent_path(), ec);
    std::ofstream(dataPath(file)) << content;
}

// "key value" lines; unknown or missing keys keep their default
Settings Settings::load()
{
    Settings settings;
    std::ifstream file(dataPath("settings"));
    std::string key;
    int value;

    while (file >> key >> value) {
        if (key == "music_volume")
            settings.music_volume = std::clamp(value, 0, 100);
        else if (key == "sfx_volume")
            settings.sfx_volume = std::clamp(value, 0, 100);
        else if (key == "jetpack_key")
            settings.jetpack_key = static_cast<sf::Keyboard::Key>(value);
        else if (key == "attack_key")
            settings.attack_key = static_cast<sf::Keyboard::Key>(value);
        else if (key == "ultimate_key")
            settings.ultimate_key = static_cast<sf::Keyboard::Key>(value);
        else if (key == "fullscreen")
            settings.fullscreen = value != 0;
    }
    return settings;
}

void Settings::save() const
{
    writeFile("settings",
              "music_volume " + std::to_string(music_volume) + "\n" +
              "sfx_volume " + std::to_string(sfx_volume) + "\n" +
              "jetpack_key " + std::to_string(static_cast<int>(jetpack_key)) + "\n" +
              "attack_key " + std::to_string(static_cast<int>(attack_key)) + "\n" +
              "ultimate_key " + std::to_string(static_cast<int>(ultimate_key)) + "\n" +
              "fullscreen " + std::to_string(fullscreen ? 1 : 0) + "\n");
}

std::string keyName(sf::Keyboard::Key key)
{
    std::string name = sf::Keyboard::getDescription(sf::Keyboard::delocalize(key)).toAnsiString();

    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    return name.empty() ? "?" : name;
}

int loadBestDistance()
{
    std::ifstream file(dataPath("best_run"));
    int distance = 0;

    file >> distance;
    return distance;
}

void saveBestDistance(int distance)
{
    writeFile("best_run", std::to_string(distance) + "\n");
}
