/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include "generator.hpp"
#include "../common/debug.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

// A fresh run: the map file if there is one (the run carries on procedurally past its end),
// otherwise a short empty runway
bool Server::resetMap()
{
    game_map.clear();
    chunks_generated = 0;

    if (!map_path.empty()) {
        if (!game_map.loadFromFile(map_path)) {
            std::cerr << "Invalid map file: " << map_path << std::endl;
            return false;
        }
    } else {
        game_map.appendChunk(std::vector<std::string>(MAP_HEIGHT, std::string(SAFE_START, '_')));
    }
    extendMap(0);
    return true;
}

void Server::extendMap(float leader_x)
{
    while (game_map.getWidth() < leader_x + LOOKAHEAD) {
        size_t from = game_map.getWidth();
        float difficulty = std::min(1.0f, chunks_generated++ / static_cast<float>(CHUNKS_TO_MAX_DIFFICULTY));

        game_map.appendChunk(generateChunk(rng, CHUNK_WIDTH, game_map.getHeight(), difficulty));

        // Before the run, clients get the whole map in one go instead
        if (phase == Phase::Running)
            broadcastToAllClients(Protocol::createPacket(MSG_MAP_DATA, game_map.serialize(from)));
    }
}

void Server::updateGameState()
{
    switch (phase) {
        case Phase::Waiting:
            if (players.size() == required_players)
                startGame();
            break;
        case Phase::Countdown:
            updateCountdown();
            break;
        case Phase::Running:
            updatePlayers();
            break;
        case Phase::Over:
            // Lobby reopens once everyone has jacked out
            if (players.empty()) {
                resetMap();
                phase = Phase::Waiting;
                std::cout << "Lobby open" << std::endl;
            }
            break;
    }
}

void Server::startGame()
{
    int player_num = 0;
    std::vector<uint8_t> roster = { static_cast<uint8_t>(players.size()) };

    for (auto &[client_fd, player] : players) {
        player.reset(player_num, game_map.getHeight() - Player::HEIGHT);
        roster.push_back(player_num++);
        roster.push_back(player.getCharacter());
    }

    roster.push_back(district);

    // Each client gets its own number first, then who plays which character, then the district
    for (auto &[client_fd, player] : players) {
        std::vector<uint8_t> payload = { static_cast<uint8_t>(player.getPlayerNumber()) };
        payload.insert(payload.end(), roster.begin(), roster.end());
        sendToClient(client_fd, Protocol::createPacket(MSG_GAME_START, payload));
    }

    phase = Phase::Countdown;
    countdown = COUNTDOWN_SECONDS;
    countdown_ticks = 0;
    run_speed = START_SPEED;
    ready_clients.clear();

    std::cout << "Game starting with " << players.size() << " players" << std::endl;
}

// Rematch once everyone in the lobby asked for one
void Server::handleReady(int client_fd)
{
    if (phase != Phase::Over)
        return;

    ready_clients.insert(client_fd);
    if (players.size() < required_players || ready_clients.size() < players.size())
        return;

    resetMap();
    for (auto &pair : players)
        sendMapToClient(pair.first);
    startGame();
}

void Server::updateCountdown()
{
    if (countdown_ticks > 0) {
        countdown_ticks--;
        return;
    }

    broadcastToAllClients(Protocol::createPacket(MSG_COUNTDOWN, { static_cast<uint8_t>(countdown) }));
    sendGameState();

    if (countdown == 0) {
        phase = Phase::Running;
        return;
    }
    countdown--;
    countdown_ticks = TICK_RATE - 1;
}

// The run never ends on its own: it stops at the first death
void Server::updatePlayers()
{
    float max_y = game_map.getHeight() - Player::HEIGHT;
    float leader_x = 0;
    std::vector<int> survivors;

    run_speed = std::min(MAX_SPEED, run_speed + ACCELERATION * TICK_DT);

    for (auto &[client_fd, player] : players) {
        player.update(TICK_DT, run_speed, max_y);
        leader_x = std::max(leader_x, player.getX());

        if (!checkPlayerCollisions(player))
            survivors.push_back(player.getPlayerNumber());
    }

    extendMap(leader_x);
    sendGameState();

    if (survivors.size() < players.size())
        endGame(survivors.size() == 1 && !solo ? survivors[0] : -1);
}

// Samples the body from head to feet. Returns true when the player got zapped.
bool Server::checkPlayerCollisions(Player &player)
{
    float px = player.getX() + 0.5f;

    for (float dy : {0.3f, 1.0f, 1.7f}) {
        float py = player.getY() + dy;
        int tile_x = static_cast<int>(px);
        int tile_y = static_cast<int>(py);
        char tile = game_map.getTile(tile_x, tile_y);

        if (tile == 'c' && player.collectCoin(tile_x, tile_y)) {
            notifyCollision('c', player, tile_x, tile_y);
        } else if (tile == 'e' && std::abs(px - tile_x - 0.5f) < 0.4f && std::abs(py - tile_y - 0.5f) < 0.4f) {
            // Zappers only bite near their core, grazing the tile edge is fine
            notifyCollision('e', player, tile_x, tile_y);
            return true;
        }
    }
    return false;
}

void Server::sendGameState()
{
    std::vector<uint8_t> state_data;

    for (const auto &[client_fd, player] : players) {
        state_data.push_back(player.getPlayerNumber());
        Protocol::pushU32(state_data, player.getX() * POSITION_SCALE);
        Protocol::pushU16(state_data, player.getY() * POSITION_SCALE);
        Protocol::pushU16(state_data, player.getScore());
        state_data.push_back(player.isJetActive() ? 1 : 0);
    }

    broadcastToAllClients(Protocol::createPacket(MSG_GAME_STATE, state_data));
}

void Server::handlePlayerInput(int client_fd, bool jet_activated)
{
    auto it = players.find(client_fd);

    if (it == players.end())
        return;

    it->second.setJetActive(jet_activated);
    DEBUG_LOG("Player " + std::to_string(it->second.getPlayerNumber()) +
              " jet " + (jet_activated ? "ON" : "OFF"));
}

void Server::notifyCollision(char collision_type, const Player &player, int x, int y)
{
    std::vector<uint8_t> collision_data;

    collision_data.push_back(collision_type);
    collision_data.push_back(player.getPlayerNumber());
    Protocol::pushU32(collision_data, x);
    Protocol::pushU16(collision_data, y);

    broadcastToAllClients(Protocol::createPacket(MSG_COLLISION, collision_data));
}

void Server::endGame(int winner_number)
{
    if (phase != Phase::Countdown && phase != Phase::Running)
        return;

    phase = Phase::Over;

    uint8_t winner = winner_number >= 0 ? winner_number : 0xFF;
    broadcastToAllClients(Protocol::createPacket(MSG_GAME_END, { winner }));

    if (winner_number >= 0)
        std::cout << "Game over, player " << winner_number << " wins" << std::endl;
    else
        std::cout << "Game over, no winner" << std::endl;
}
