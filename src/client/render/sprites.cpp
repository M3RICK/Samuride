// Every picture of the game, loaded once and shared by the menu and the run

#include "sprites.hpp"
#include "canvas.hpp"
#include "../../common/titans.hpp"
#include "../../common/debug.hpp"
#include <algorithm>

void Sprites::load()
{
    static const char *titan_dirs[TITAN_TYPES] = {"titan_female", "titan_armored"};

    loadTexture(poster, "assets/waiting_screen/Samuride.png");
    loadTexture(coin, "assets/coins/coin.png");
    loadTexture(zapper, "assets/electric/zap.png");
    // Pixel art: scaled up without smoothing
    loadTexture(drone, "assets/drone/drone.png", false);

    characters.resize(CHARACTER_DIRS.size());
    for (size_t i = 0; i < CHARACTER_DIRS.size(); i++) {
        if (!characters[i].load("assets/" + CHARACTER_DIRS[i]))
            DEBUG_LOG("Failed to load character: " + CHARACTER_DIRS[i]);
    }

    if (!shift.load("assets/eren_shift") || !eren_titan.load("assets/titan_eren") ||
        !titan_face.loadFromFile("assets/titan_eren/portrait.png"))
        DEBUG_LOG("Failed to load the Attack Titan");

    titans.resize(TITAN_TYPES);
    for (int i = 0; i < TITAN_TYPES; i++) {
        if (!titans[i].load(std::string("assets/") + titan_dirs[i]))
            DEBUG_LOG(std::string("Failed to load titan: ") + titan_dirs[i]);
    }
}

const Character &Sprites::character(int id) const
{
    return characters[std::clamp(id, 0, static_cast<int>(characters.size()) - 1)];
}
