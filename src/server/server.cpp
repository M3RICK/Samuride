#include "server.hpp"
#include "player.hpp"
#include "../common/debug.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <algorithm>

Server::Server(int port, const std::string& map_path, bool debug_mode)
    : server_fd(-1), port(port), map_path(map_path), debug_mode(debug_mode),
      game_started(false) {
    g_logger.setDebugMode(debug_mode);
}

Server::~Server() {
    // Clean up players
    for (auto& pair : players) {
        delete pair.second;
    }
    players.clear();

    // Close all connections
    for (auto& pfd : poll_fds) {
        if (pfd.fd >= 0) {
            close(pfd.fd);
        }
    }
}

bool Server::initialize() {
    // Load map
    if (!game_map.loadFromFile(map_path)) {
        std::cerr << "Failed to load map: " << map_path << std::endl;
        return false;
    }

    // Initialize server socket
    return initializeServer();
}

bool Server::initializeServer()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Set socket to non-blocking
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options." << std::endl;
        close(server_fd);
        return false;
    }

    // Bind to port
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        close(server_fd);
        return false;
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(server_fd);
        return false;
    }

    // Add server socket to poll array
    pollfd pfd = {server_fd, POLLIN, 0};
    poll_fds.push_back(pfd);

    std::cout << "Server started on port " << port << std::endl;
    DEBUG_LOG("Server initialized with debug mode " + std::string(debug_mode ? "enabled" : "disabled"));

    return true;
}

void Server::run() {
    while (true) {
        // Poll for events
        int poll_count = poll(poll_fds.data(), poll_fds.size(), 100);

        if (poll_count < 0) {
            std::cerr << "Poll error." << std::endl;
            break;
        }

        // Check for events on each socket
        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents == 0) {
                continue;
            }

            // New connection on server socket
            if (poll_fds[i].fd == server_fd && (poll_fds[i].revents & POLLIN)) {
                acceptNewClient();
                continue;
            }

            // Data from client
            if (poll_fds[i].revents & POLLIN) {
                handleClientData(poll_fds[i].fd);
            }

            // Client disconnected
            if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                removeClient(poll_fds[i].fd);
            }
        }

        // If game has started, update game state
        if (game_started) {
            checkGameState();
        }
        // If we have at least 2 players and game not started, check if we should start
        else if (players.size() >= 2 && !game_started) {
            startGame();
        }
    }
}

void Server::acceptNewClient() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);

    if (client_fd < 0) {
        std::cerr << "Failed to accept client connection." << std::endl;
        return;
    }

    // Set client socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // Add client to poll array
    pollfd pfd = {client_fd, POLLIN, 0};
    poll_fds.push_back(pfd);

    // Create player
    players[client_fd] = new Player(client_fd);

    std::cout << "New client connected: " << client_fd - 3 << std::endl;
    DEBUG_LOG("Client connected: fd=" + std::to_string(client_fd));

    // Send map data to client
    std::vector<uint8_t> map_data = game_map.serialize();
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_MAP_DATA, map_data);
    sendToClient(client_fd, packet);

    // If game has already started, don't allow more players to join
    if (game_started) {
        DEBUG_LOG("Game already started, disconnecting client: " + std::to_string(client_fd));
        removeClient(client_fd);
    }
}

void Server::handleClientData(int client_fd) {
    // Clear buffer
    memset(recv_buffer, 0, BUFFER_SIZE);

    // Receive data
    ssize_t bytes_read = recv(client_fd, recv_buffer, BUFFER_SIZE, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            DEBUG_LOG("Client disconnected: " + std::to_string(client_fd));
            removeClient(client_fd);
        } else {
            std::cerr << "Error reading from client: " << client_fd << std::endl;
            DEBUG_LOG("Error reading from client: " + std::to_string(client_fd) + ", errno=" + std::to_string(errno));
        }
        return;
    }

    DEBUG_PACKET_RECV(recv_buffer, bytes_read);

    // Parse header
    MessageHeader header;
    if (!Protocol::parseHeader(recv_buffer, bytes_read, header)) {
        DEBUG_LOG("Invalid packet from client: " + std::to_string(client_fd));
        return;
    }

    uint32_t payload_size = Protocol::getPayloadSize(header);

    // Check if we have a complete packet
    if (bytes_read < sizeof(MessageHeader) + payload_size) {
        DEBUG_LOG("Incomplete packet from client: " + std::to_string(client_fd));
        return;
    }

    switch (header.type) {
        case MSG_CONNECT:
            DEBUG_LOG("Client " + std::to_string(client_fd) + " sent connect message");
            // Player is already connected by this point
            break;

        case MSG_PLAYER_INPUT:
            if (payload_size >= 1) {
                bool jet_activated = (recv_buffer[sizeof(MessageHeader)] != 0);
                handlePlayerInput(client_fd, jet_activated);
            }
            break;

        default:
            DEBUG_LOG("Unknown message type from client: " + std::to_string(client_fd) +
                      ", type=" + std::to_string(header.type));
            break;
    }
}

void Server::sendToClient(int client_fd, const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    ssize_t bytes_sent = send(client_fd, data.data(), data.size(), 0);

    if (bytes_sent < 0) {
        std::cerr << "Failed to send data to client: " << client_fd << std::endl;
        DEBUG_LOG("Failed to send data to client: " + std::to_string(client_fd) +
                  ", errno=" + std::to_string(errno));
        return;
    }

    DEBUG_PACKET_SEND(reinterpret_cast<const char*>(data.data()), bytes_sent);
}

void Server::broadcastToAllClients(const std::vector<uint8_t>& data)
{
    for (auto& pair : players) {
        sendToClient(pair.first, data);
    }
}

void Server::removeClient(int client_fd)
{
    close(client_fd);

    auto it = std::find_if(poll_fds.begin(), poll_fds.end(),
                          [client_fd](const pollfd& pfd) { return pfd.fd == client_fd; });
    if (it != poll_fds.end()) {
        poll_fds.erase(it);
    }

    auto player_it = players.find(client_fd);

    if (player_it != players.end()) {
        delete player_it->second;
        players.erase(player_it);
    }

    if (game_started && players.size() < 2) {
        // End game if only one player left
        if (players.size() == 1) {
            endGame(players.begin()->first);
        } else {
            // No players left, reset game
            game_started = false;
        }
    }

    DEBUG_LOG("Client removed: " + std::to_string(client_fd));
}

void Server::startGame() {
    if (players.size() < 2) {
        return;
    }

    game_started = true;

    int player_num = 0;

    for (auto& pair : players) {
        Player* player = pair.second;
        player->setX(0);
        player->setY(game_map.getHeight() / 2);
        player->setPlayerNumber(player_num++);
    }

    std::vector<uint8_t> payload;
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_GAME_START, payload);
    broadcastToAllClients(packet);

    DEBUG_LOG("Game started with " + std::to_string(players.size()) + " players");
}

void Server::checkGameState()
{
    bool game_over = false;
    int winner_fd = -1;

    for (auto& pair : players) {
        Player* player = pair.second;

        // Move player forward automatically
        player->moveForward();

        // Apply gravity or jetpack
        player->applyPhysics();

        // Check map boundaries
        if (player->getY() < 0) {
            player->setY(0);
        } else if (player->getY() >= game_map.getHeight()) {
            player->setY(game_map.getHeight() - 1);
        }

        // Check for collisions
        char tile = game_map.getTile(player->getX(), player->getY());
        if (tile == 'c') {
            // Coin collected
            player->addScore(1);
            notifyCollision(pair.first, 'c', player->getX(), player->getY());
        } else if (tile == 'e') {
            // Electric hazard - player dies
            game_over = true;
            // Other player wins
            for (auto& other_pair : players) {
                if (other_pair.first != pair.first) {
                    winner_fd = other_pair.first;
                    break;
                }
            }
            break;
        }

        // Check if player reached the end of the map
        if (player->getX() >= game_map.getWidth()) {
            game_over = true;
            winner_fd = pair.first;
            break;
        }
    }

    std::vector<uint8_t> state_data;
    for (auto& pair : players) {
        Player* player = pair.second;

        // Add player number (1 byte)
        state_data.push_back(player->getPlayerNumber());

        uint16_t x = player->getX();
        state_data.push_back((x >> 8) & 0xFF);
        state_data.push_back(x & 0xFF);

        uint16_t y = player->getY();
        state_data.push_back((y >> 8) & 0xFF);
        state_data.push_back(y & 0xFF);

        uint16_t score = player->getScore();
        state_data.push_back((score >> 8) & 0xFF);
        state_data.push_back(score & 0xFF);

        state_data.push_back(player->isJetActive() ? 1 : 0);
    }

    std::vector<uint8_t> state_packet = Protocol::createPacket(MSG_GAME_STATE, state_data);
    broadcastToAllClients(state_packet);

    if (game_over) {
        endGame(winner_fd);
    }
}

void Server::handlePlayerInput(int client_fd, bool jet_activated)
{
    auto it = players.find(client_fd);
    if (it == players.end()) {
        DEBUG_LOG("Player input from unknown client: " + std::to_string(client_fd));
        return;
    }

    it->second->setJetActive(jet_activated);
}

void Server::notifyCollision(int client_fd, char collision_type, int x, int y)
{
    std::vector<uint8_t> collision_data;

    collision_data.push_back(collision_type);

    uint16_t pos_x = x;
    uint16_t pos_y = y;
    collision_data.push_back((pos_x >> 8) & 0xFF);
    collision_data.push_back(pos_x & 0xFF);
    collision_data.push_back((pos_y >> 8) & 0xFF);
    collision_data.push_back(pos_y & 0xFF);

    std::vector<uint8_t> collision_packet = Protocol::createPacket(MSG_COLLISION, collision_data);
    broadcastToAllClients(collision_packet);
}

void Server::endGame(int winner_fd)
{
    if (!game_started) {
        return;
    }

    std::vector<uint8_t> end_data;

    if (winner_fd >= 0 && players.find(winner_fd) != players.end()) {
        end_data.push_back(players[winner_fd]->getPlayerNumber());
    } else {
        end_data.push_back(0xFF); // No winner
    }

    std::vector<uint8_t> end_packet = Protocol::createPacket(MSG_GAME_END, end_data);
    broadcastToAllClients(end_packet);

    game_started = false;

    DEBUG_LOG("Game ended, winner: " + (winner_fd >= 0 ? std::to_string(winner_fd) : "none"));
}
