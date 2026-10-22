// The Attack Titan power-up: orbs placed in the map, and what the titan form smashes

#include "server.hpp"

// Now and then an orb floats in an empty tile of a new chunk
void Server::placeShiftOrb(std::vector<std::string> &rows)
{
    std::uniform_real_distribution<float> chance(0, 1);

    bool too_soon = chunks_generated < SHIFT_FIRST_CHUNK || chunks_generated - last_orb_chunk < SHIFT_MIN_GAP_CHUNKS;

    if (!too_soon && chance(rng) < SHIFT_CHANCE && placeInFreeTile(rows, SHIFT_TILE))
        last_orb_chunk = chunks_generated;
}

// Everything in the titan's body: coins collected, drones and titans smashed
void Server::titanSmash(Player &player)
{
    int left = static_cast<int>(player.getX());
    int right = static_cast<int>(player.getX() + TITAN_FORM_WIDTH);
    int floor = game_map.getHeight();
    int top = static_cast<int>(floor - TITAN_FORM_HEIGHT);

    for (int x = left; x <= right; x++) {
        for (int y = top; y < floor; y++) {
            if (game_map.getTile(x, y) == COIN_TILE && player.collectCoin(x, y))
                notifyCollision(COIN_TILE, player, x, y);
            else if (isLiveDrone(x, y))
                destroyDrone(player.getClientFd(), x, y);
        }
    }

    auto reached = [&](const Horde::Titan &titan) {
        return titan.x < player.getX() + TITAN_FORM_WIDTH && titan.x + titanStats(titan.type).width > player.getX();
    };
    for (float x : titans.takeDown(reached))
        titanDown(player, x);
}
