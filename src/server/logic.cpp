/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include "player.hpp"
#include "../common/debug.hpp"

void Server::startGame()
{
    if (players.size() < 2)
        return;

    DEBUG_LOG("Starting game countdown with " + std::to_string(players.size()) + " players");

    // Ca marche qu a moitie
    for (int count = 3; count >= 0; count--) {

        std::vector<uint8_t> payload = { static_cast<uint8_t>(count) };
        std::vector<uint8_t> packet = Protocol::createPacket(MSG_COUNTDOWN, payload);

        broadcastToAllClients(packet);

        if (count > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::vector<uint8_t> startPayload;
    std::vector<uint8_t> startPacket = Protocol::createPacket(MSG_GAME_START, startPayload);

    broadcastToAllClients(startPacket);

    game_started = true;

    initializePlayerPositions();

    DEBUG_LOG("Game started with " + std::to_string(players.size()) + " players");
}

void Server::initializePlayerPositions()
{
    int player_num = 0;

    for (auto &pair : players) {
        Player *player = pair.second;
        player->setX(0);
        player->setY(game_map.getHeight() - 3);
        player->setPlayerNumber(player_num++);
    }
 }

void Server::checkGameState()
{
    checkGameOverConditions();
    updateAndSendGameState();
}

void Server::checkGameOverConditions()
{
    for (auto &pair : players) {

        Player *player = pair.second;

        player->moveForward();
        player->applyPhysics();

        constrainPlayerToMap(player);
        checkPlayerCollisions(pair.first, player);

        if (player->getX() >= game_map.getWidth()) {
            endGame(pair.first); //deter qui win
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

bool Server::checkPlayerCollisions(int client_fd, Player *player)
{
    char tile = game_map.getTile(player->getX(), player->getY());

    if (tile == 'c') {
        player->addScore(1);
        notifyCollision(client_fd, 'c', player->getX(), player->getY());
    } else if (tile == 'e') {
        notifyCollision(client_fd, 'e', player->getX(), player->getY());
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

    DEBUG_LOG("Updating game state for " + std::to_string(players.size()) + " players");

    for (auto& pair : players) {
        Player* player = pair.second;

        DEBUG_LOG("Player " + std::to_string(player->getPlayerNumber()) +
                  " state: pos=(" + std::to_string(player->getX()) + "," +
                  std::to_string(player->getY()) + "), jet=" +
                  (player->isJetActive() ? "ON" : "OFF"));

        addPlayerStateToPacket(state_data, player);
    }

    std::vector<uint8_t> state_packet = Protocol::createPacket(MSG_GAME_STATE, state_data);

    broadcastToAllClients(state_packet);
}

void Server::addPlayerStateToPacket(std::vector<uint8_t> &data, Player *player)
{
    data.push_back(player->getPlayerNumber());

    // X position 2 bytes
    uint16_t x = player->getX();
    data.push_back((x >> 8) & 0xFF);
    data.push_back(x & 0xFF);

    // Y position 2 bytes
    uint16_t y = player->getY();
    data.push_back((y >> 8) & 0xFF);
    data.push_back(y & 0xFF);

    // Score 2 bytes
    uint16_t score = player->getScore();
    data.push_back((score >> 8) & 0xFF);
    data.push_back(score & 0xFF);

    // Jetpack state 1 byte
    data.push_back(player->isJetActive() ? 1 : 0);
}

void Server::handlePlayerInput(int client_fd, bool jet_activated)
{
    auto it = players.find(client_fd);

    if (it == players.end()) {
        DEBUG_LOG("Unknown input, dont try cheatin: " + std::to_string(client_fd));
        return;
    }

    Player *player = it->second;
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

    collision_data.push_back(collision_type);

    // Position 4 bytes total = x and y coordinates 2 + 2
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

    // identify winner or 0xFF for no winner
    if (winner_fd >= 0 && players.find(winner_fd) != players.end()) {
        end_data.push_back(players[winner_fd]->getPlayerNumber());
    } else {
        end_data.push_back(0xFF);
    }

    std::vector<uint8_t> end_packet = Protocol::createPacket(MSG_GAME_END, end_data);
    broadcastToAllClients(end_packet);

    game_started = false;

    DEBUG_LOG("ITS OVER, WINNER IS: " + (winner_fd >= 0 ? std::to_string(players[winner_fd]->getPlayerNumber()) : "No winner ? You both suck"));
}
