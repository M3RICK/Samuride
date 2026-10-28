// Shield, Sandevistan and Berserk: placed in the map, dropped by drones, and the shield taking a hit

#include "server.hpp"

// A few tries to land on free space, away from the chunk's edges
bool Server::placeInFreeTile(std::vector<std::string> &rows, char tile)
{
    std::uniform_int_distribution<int> column(5, rows[0].size() - 5);
    std::uniform_int_distribution<int> row(2, rows.size() - 3);

    for (int attempt = 0; attempt < 10; attempt++) {
        char &spot = rows[row(rng)][column(rng)];
        if (spot == EMPTY_TILE) {
            spot = tile;
            return true;
        }
    }
    return false;
}

void Server::placePowerUp(std::vector<std::string> &rows)
{
    std::uniform_real_distribution<float> chance(0, 1);

    float odds = mode == GameMode::Cyberpsycho ? POWERUP_CHANCE_CYBERPSYCHO : POWERUP_CHANCE;

    if (chunks_generated < POWERUP_FIRST_CHUNK || chance(rng) >= odds)
        return;
    // Any power-up, but Berserk is no use without attacks: CYBERPSYCHO only
    std::vector<char> kinds;
    for (char tile : POWER_UPS) {
        if (tile != BERSERK_TILE || mode == GameMode::Cyberpsycho)
            kinds.push_back(tile);
    }
    placeInFreeTile(rows, kinds[std::uniform_int_distribution<size_t>(0, kinds.size() - 1)(rng)]);
}

// What a player can pick up on this tile: a power-up of the map, or one a destroyed drone dropped
char Server::powerUpAt(int x, int y) const
{
    char tile = game_map.getTile(x, y);

    if (isPowerUp(tile))
        return tile;
    if (tile == DRONE_TILE && destroyed_drones.count({x, y}))
        return droneDrop(x, y, pinata_drops.count({x, y}));
    return EMPTY_TILE;
}

// A deadly hit: the shield takes it if there is one, the Trauma Team comes if it can. Returns true when
// the player dies.
bool Server::hurt(Player &player, char cause, int x, int y)
{
    if (player.absorbHit()) {
        notifyCollision(SHIELD_BREAK, player, x, y);
        return false;
    }
    if (player.callTraumaTeam()) {
        notifyCollision(TRAUMA_RESCUE, player, x, y);
        return false;
    }
    notifyCollision(cause, player, x, y);
    return true;
}
