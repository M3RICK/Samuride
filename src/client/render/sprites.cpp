// Every picture of the game, loaded once and shared by the menu and the run

#include "sprites.hpp"
#include "canvas.hpp"
#include "../../common/titans.hpp"
#include "../../common/runners.hpp"
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
    loadTexture(hack_target, "assets/lucy/target.png");
    loadTexture(fishbones, "assets/jinx/fishbones.png", false);

    characters.resize(RUNNER_COUNT);
    for (int i = 0; i < RUNNER_COUNT; i++) {
        if (!characters[i].load(std::string("assets/") + RUNNERS[i].dir))
            DEBUG_LOG(std::string("Failed to load runner: ") + RUNNERS[i].dir);
    }

    if (!shift.load("assets/eren_shift") || !eren_titan.load("assets/titan_eren") ||
        !titan_face.loadFromFile("assets/titan_eren/portrait.png"))
        DEBUG_LOG("Failed to load the Attack Titan");

    if (!smasher.load("assets/smasher"))
        DEBUG_LOG("Failed to load Adam Smasher");
    if (!trauma_av.load("assets/trauma_av") || !trauma_medic.load("assets/trauma_medic"))
        DEBUG_LOG("Failed to load the Trauma Team");

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
