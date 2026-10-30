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

// A deadly hit: the shield takes it if there is one, the Trauma Team offers to come if it can. Returns true
// when the player dies.
bool Server::hurt(Player &player, char cause, int x, int y)
{
    if (player.absorbHit()) {
        notifyCollision(SHIELD_BREAK, player, x, y);
        return false;
    }
    // The Trauma Team asks first: the run waits for the answer (answerTraumaTeam)
    if (player.canCallTraumaTeam()) {
        if (trauma_offers.try_emplace(player.getClientFd(), TraumaOffer{cause, x, y, TRAUMA_OFFER_TIME}).second)
            notifyCollision(TRAUMA_OFFER, player, x, y);
        return false;
    }
    notifyCollision(cause, player, x, y);
    return true;
}

// Each runner's three cards: a general boon, another one or their own, a pact
void Server::offerBoons()
{
    auto roll = [&](int from, int to) { return static_cast<Boon>(std::uniform_int_distribution<int>(from, to - 1)(rng)); };
    boon_left = BOON_TIME;
    for (auto &[client_fd, player] : players) {
        Boon first = roll(1, FIRST_PACT);
        Boon second = first;
        while (second == first)
            second = roll(1, FIRST_PACT);
        if (std::uniform_int_distribution<int>(0, 1)(rng))
            second = Boon::Signature;
        std::array<Boon, 3> cards{first, second, roll(FIRST_PACT, PACTS_END)};
        boon_offers[client_fd] = cards;
        int ids = static_cast<int>(cards[0]) | static_cast<int>(cards[1]) << 8 | static_cast<int>(cards[2]) << 16;
        notifyCollision(BOON_OFFER, player, ids, 0);
    }
}

// Card 1 to 3, or 0 for none; the run goes on once everyone has picked
void Server::answerBoon(int client_fd, int card)
{
    auto offer = boon_offers.find(client_fd);
    if (offer == boon_offers.end())
        return;
    Boon boon = card >= 1 && card <= 3 ? offer->second[card - 1] : Boon::None;
    boon_offers.erase(offer);
    Player &player = players.at(client_fd);
    player.takeBoon(boon);
    notifyCollision(BOON_TAKEN, player, static_cast<int>(boon), 0);
}

// One implant, or none, and the run goes on once everyone has picked
void Server::answerRipperdoc(int client_fd, int implant)
{
    if (!ripperdoc_waiting.count(client_fd))
        return;
    Player &player = players.at(client_fd);
    // One they can't afford: still choosing
    if (implant != 0 && !player.buyImplant(implant))
        return;
    ripperdoc_waiting.erase(client_fd);
    // x = the implant, 0 for none: their clinic closes either way
    notifyCollision(IMPLANT_INSTALLED, player, implant, 0);
}

void Server::answerTraumaTeam(int client_fd, bool call)
{
    auto offer = trauma_offers.find(client_fd);
    if (offer == trauma_offers.end())
        return;
    TraumaOffer hit = offer->second;
    trauma_offers.erase(offer);
    Player &player = players.at(client_fd);

    if (call && player.callTraumaTeam()) {
        notifyCollision(TRAUMA_RESCUE, player, hit.x, hit.y);
        return;
    }
    notifyCollision(hit.cause, player, hit.x, hit.y);
    std::vector<int> survivors;
    for (const auto &[fd, other] : players) {
        if (fd != client_fd)
            survivors.push_back(other.getPlayerNumber());
    }
    endGame(survivors.size() == 1 && !config.solo ? survivors[0] : -1);
}
