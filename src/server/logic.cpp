/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include "player.hpp"
#include "../common/debug.hpp"

/**
 * Start the game
 */
void Server::startGame() {
    if (players.size() < 2) {
        return; // Need at least 2 players
    }

    DEBUG_LOG("Starting game countdown with " + std::to_string(players.size()) + " players");

    // Send a countdown to clients (3...2...1...GO!)
    for (int count = 3; count >= 0; count--) {
        // Create a special countdown packet
        std::vector<uint8_t> payload = { static_cast<uint8_t>(count) };
        std::vector<uint8_t> packet = Protocol::createPacket(MSG_COUNTDOWN, payload);

        // Broadcast to all clients
        broadcastToAllClients(packet);

        if (count > 0) {
            // Sleep between countdown steps
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // Create and send the actual game start message
    std::vector<uint8_t> startPayload;
    std::vector<uint8_t> startPacket = Protocol::createPacket(MSG_GAME_START, startPayload);
    broadcastToAllClients(startPacket);

    // Set the game state to started
    game_started = true;

    // Initialize player positions
    initializePlayerPositions();

    DEBUG_LOG("Game started with " + std::to_string(players.size()) + " players");
}

/**
 * Initialize positions for all players
 */
void Server::initializePlayerPositions()
{
    int player_num = 0;

    for (auto& pair : players) {
        Player* player = pair.second;
        player->setX(0);
        player->setY(game_map.getHeight() - 3);
        player->setPlayerNumber(player_num++);
    }
 }

/**
 * Check and update game state
 */
void Server::checkGameState()
{
    // Check for win/lose conditions
    checkGameOverConditions();

    // Update and send game state to clients
    updateAndSendGameState();
}

/**
 * Check if the game is over due to win/lose conditions
 */
void Server::checkGameOverConditions()
{
    for (auto& pair : players) {
        Player* player = pair.second;

        // Move player forward automatically
        player->moveForward();

        // Apply gravity or jetpack
        player->applyPhysics();

        // Keep player within map boundaries
        constrainPlayerToMap(player);

        // Check for collisions with map elements
        checkPlayerCollisions(pair.first, player);

        // Check if player reached the end of the map
        if (player->getX() >= game_map.getWidth()) {
            endGame(pair.first); // This player wins
            return;
        }
    }
}

void Server::constrainPlayerToMap(Player *player)
{
    if (player->getY() < 0) {
        player->setY(0);
    } else if (player->getY() >= game_map.getHeight()) {
        player->setY(game_map.getHeight() - 1);
    }
}

/**
 * Check for player collisions with map elements
 * @param client_fd The client socket
 * @param player The player to check
 * @return true if game should end, false otherwise
 */
bool Server::checkPlayerCollisions(int client_fd, Player* player)
{
    char tile = game_map.getTile(player->getX(), player->getY());

    if (tile == 'c') {
        // Coin collected
        player->addScore(1);
        notifyCollision(client_fd, 'c', player->getX(), player->getY());
    } else if (tile == 'e') {
        // Electric hazard - player dies
        notifyCollision(client_fd, 'e', player->getX(), player->getY());

        // Find other player to declare as winner
        for (auto &other_pair : players) {
            if (other_pair.first != client_fd) {
                endGame(other_pair.first);
                return true;
            }
        }
    }

    return false;
}

void Server::updateAndSendGameState()
{
    std::vector<uint8_t> state_data;

    // Add all players' state to the packet
    DEBUG_LOG("Updating game state for " + std::to_string(players.size()) + " players");

    for (auto& pair : players) {
        Player* player = pair.second;

        // Log each player's state before sending
        DEBUG_LOG("Player " + std::to_string(player->getPlayerNumber()) +
                  " state: pos=(" + std::to_string(player->getX()) + "," +
                  std::to_string(player->getY()) + "), jet=" +
                  (player->isJetActive() ? "ON" : "OFF"));

        addPlayerStateToPacket(state_data, player);
    }

    // Send state to all clients
    std::vector<uint8_t> state_packet = Protocol::createPacket(MSG_GAME_STATE, state_data);
    broadcastToAllClients(state_packet);
}

void Server::addPlayerStateToPacket(std::vector<uint8_t>& data, Player* player)
{
    // Add player number (1 byte)
    data.push_back(player->getPlayerNumber());

    // Add X position (2 bytes)
    uint16_t x = player->getX();
    data.push_back((x >> 8) & 0xFF);
    data.push_back(x & 0xFF);

    // Add Y position (2 bytes)
    uint16_t y = player->getY();
    data.push_back((y >> 8) & 0xFF);
    data.push_back(y & 0xFF);

    // Add score (2 bytes)
    uint16_t score = player->getScore();
    data.push_back((score >> 8) & 0xFF);
    data.push_back(score & 0xFF);

    // Add jetpack state (1 byte)
    data.push_back(player->isJetActive() ? 1 : 0);
}

void Server::handlePlayerInput(int client_fd, bool jet_activated)
{
    auto it = players.find(client_fd);
    if (it == players.end()) {
        DEBUG_LOG("Player input from unknown client: " + std::to_string(client_fd));
        return;
    }

    Player* player = it->second;
    int player_number = player->getPlayerNumber();

    DEBUG_LOG("INPUT: client_fd=" + std::to_string(client_fd) +
              ", player_number=" + std::to_string(player_number) +
              ", jet=" + (jet_activated ? "ON" : "OFF"));

    player->setJetActive(jet_activated);

    for (const auto& p : players) {
        DEBUG_LOG("PLAYER STATE: client_fd=" + std::to_string(p.first) +
                  ", player_number=" + std::to_string(p.second->getPlayerNumber()) +
                  ", jet=" + (p.second->isJetActive() ? "ON" : "OFF"));
    }
}

void Server::notifyCollision(int client_fd, char collision_type, int x, int y)
{
    std::vector<uint8_t> collision_data;

    // Add collision type
    collision_data.push_back(collision_type);

    // Add position (4 bytes total)
    uint16_t pos_x = x;
    uint16_t pos_y = y;
    collision_data.push_back((pos_x >> 8) & 0xFF);
    collision_data.push_back(pos_x & 0xFF);
    collision_data.push_back((pos_y >> 8) & 0xFF);
    collision_data.push_back(pos_y & 0xFF);

    // Create and send collision packet
    std::vector<uint8_t> collision_packet = Protocol::createPacket(MSG_COLLISION, collision_data);
    broadcastToAllClients(collision_packet);
}

void Server::endGame(int winner_fd)
{
    if (!game_started)
        return;
    std::vector<uint8_t> end_data;

    // Add winner player number (or 0xFF for no winner)
    if (winner_fd >= 0 && players.find(winner_fd) != players.end()) {
        end_data.push_back(players[winner_fd]->getPlayerNumber());
    } else {
        end_data.push_back(0xFF); // No winner
    }

    // Create and send game end packet
    std::vector<uint8_t> end_packet = Protocol::createPacket(MSG_GAME_END, end_data);
    broadcastToAllClients(end_packet);

    game_started = false;

    DEBUG_LOG("Game ended, winner: " + (winner_fd >= 0 ? std::to_string(players[winner_fd]->getPlayerNumber()) : "none"));
}
