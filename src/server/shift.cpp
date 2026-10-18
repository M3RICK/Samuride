/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"

// The Attack Titan power-up: orbs placed in the map, and what the titan form smashes

// Now and then an orb floats in an empty tile of a new chunk
void Server::placeShiftOrb(std::vector<std::string> &rows)
{
    std::uniform_real_distribution<float> chance(0, 1);

    if (chunks_generated < SHIFT_FIRST_CHUNK || chance(rng) >= SHIFT_CHANCE)
        return;

    std::uniform_int_distribution<int> column(5, rows[0].size() - 5);
    std::uniform_int_distribution<int> row(2, rows.size() - 3);

    // A few tries to land on free space
    for (int attempt = 0; attempt < 10; attempt++) {
        char &tile = rows[row(rng)][column(rng)];
        if (tile == '_') {
            tile = SHIFT_TILE;
            return;
        }
    }
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
            if (game_map.getTile(x, y) == 'c' && player.collectCoin(x, y))
                notifyCollision('c', player, x, y);
            else if (isLiveDrone(x, y))
                destroyDrone(player.getClientFd(), x, y);
        }
    }

    std::erase_if(titans, [&](const Titan &titan) {
        bool reached = titan.x < player.getX() + TITAN_FORM_WIDTH && titan.x + titanStats(titan.type).width > player.getX();
        if (!reached)
            return false;
        player.addScore(TITAN_BOUNTY);
        notifyCollision(TITAN_KILL, player, titan.x, floor - 1);
        return true;
    });
}
