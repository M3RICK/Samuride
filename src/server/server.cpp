/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

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

//=============================================================================
// Constructor & Destructor
//=============================================================================

Server::Server(int port, const std::string& map_path, bool debug_mode)
    : server_fd(-1), port(port), map_path(map_path), debug_mode(debug_mode),
      game_started(false) {
    g_logger.setDebugMode(debug_mode);
}

Server::~Server()
{
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

//=============================================================================
// Server Initialization
//=============================================================================

bool Server::initialize()
{
    // Load the game map
    if (!loadGameMap()) {
        return false;
    }

    // Initialize server socket
    return initializeServer();
}

bool Server::loadGameMap()
{
    if (!game_map.loadFromFile(map_path)) {
        std::cerr << "Failed to load map: " << map_path << std::endl;
        return false;
    }

    DEBUG_LOG("Map loaded successfully: " + std::to_string(game_map.getWidth()) +
              "x" + std::to_string(game_map.getHeight()));
    return true;
}

bool Server::initializeServer()
{
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Set socket to non-blocking mode
    setNonBlocking(server_fd);

    // Allow socket address reuse
    if (!setSocketOptions()) {
        close(server_fd);
        return false;
    }

    // Bind socket to address and port
    if (!bindSocket()) {
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

bool Server::setSocketOptions()
{
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options." << std::endl;
        return false;
    }
    return true;
}

bool Server::bindSocket()
{
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        return false;
    }
    return true;
}

void Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

//=============================================================================
// Main Server Loop
//=============================================================================

void Server::run()
{
    while (true) {
        // Poll for events with a 100ms timeout
        int poll_count = poll(poll_fds.data(), poll_fds.size(), 100);

        if (poll_count < 0)
            break;

        // Process events on sockets
        processSocketEvents();

        // Game logic updates
        updateGameState();
    }
}

void Server::processSocketEvents()
{
    for (size_t i = 0; i < poll_fds.size(); i++) {
        if (poll_fds[i].revents == 0) {
            continue; // No events on this socket
        }

        // Handle new connections on server socket
        if (poll_fds[i].fd == server_fd && (poll_fds[i].revents & POLLIN)) {
            acceptNewClient();
            continue;
        }

        // Handle data from client
        if (poll_fds[i].revents & POLLIN) {
            handleClientData(poll_fds[i].fd);
        }

        // Handle client disconnection
        if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
            removeClient(poll_fds[i].fd);
        }
    }
}

void Server::updateGameState()
{
    // If game has started, update game state
    if (game_started) {
        checkGameState();
    }
    // If we have at least 2 players and game not started, check if we should start
    else if (players.size() >= 2 && !game_started) {
        startGame();
    }
}

//=============================================================================
// Client Connection Management
//=============================================================================

void Server::acceptNewClient()
{
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);

    if (client_fd < 0)
        return;

    // Set client socket to non-blocking
    setNonBlocking(client_fd);

    // Add client to poll array
    pollfd pfd = {client_fd, POLLIN, 0};
    poll_fds.push_back(pfd);

    // Create player object for this client
    players[client_fd] = new Player(client_fd);

    std::cout << "New client connected: " << client_fd << std::endl;
    DEBUG_LOG("Client connected: fd=" + std::to_string(client_fd));

    // Send map data to client
    sendMapToClient(client_fd);

    // Allow a small delay for client to process map data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Don't allow joining a game in progress
    if (game_started) {
        DEBUG_LOG("Game already started, disconnecting client: " + std::to_string(client_fd));
        removeClient(client_fd);
    }
}

void Server::sendMapToClient(int client_fd)
{
    std::vector<uint8_t> map_data = game_map.serialize();
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_MAP_DATA, map_data);
    sendToClient(client_fd, packet);
}

void Server::removeClient(int client_fd)
{
    close(client_fd); // Close the socket

    // Remove from poll_fds array
    auto it = std::find_if(poll_fds.begin(), poll_fds.end(),
                          [client_fd](const pollfd& pfd) { return pfd.fd == client_fd; });
    if (it != poll_fds.end()) {
        poll_fds.erase(it);
    }

    // Delete player object and remove from players map
    auto player_it = players.find(client_fd);
    if (player_it != players.end()) {
        delete player_it->second;
        players.erase(player_it);
    }

    // Handle game state changes due to player disconnection
    handlePlayerDisconnection();

    DEBUG_LOG("Client removed: " + std::to_string(client_fd));
}

void Server::handlePlayerDisconnection()
{
    if (game_started && players.size() < 2) {
        // End game if only one player left
        if (players.size() == 1) {
            endGame(players.begin()->first);
        } else {
            // No players left, reset game
            game_started = false;
        }
    }
}

//=============================================================================
// Client Data Handling
//=============================================================================

void Server::handleClientData(int client_fd)
{
    // Clear receive buffer
    memset(recv_buffer, 0, BUFFER_SIZE);

    // Receive data
    ssize_t bytes_read = recv(client_fd, recv_buffer, BUFFER_SIZE, 0);

    // Check for disconnection or error
    if (bytes_read <= 0) {
        handleReceiveError(client_fd, bytes_read);
        return;
    }

    DEBUG_PACKET_RECV(recv_buffer, bytes_read);

    // Process the message
    processClientMessage(client_fd, bytes_read);
}


void Server::handleReceiveError(int client_fd, ssize_t bytes_read)
{
    if (bytes_read == 0) {
        DEBUG_LOG("Client disconnected: " + std::to_string(client_fd));
        removeClient(client_fd);
    } else {
        DEBUG_LOG("Error reading from client: " + std::to_string(client_fd) +
                ", errno=" + std::to_string(errno));
    }
}

bool Server::parseMessageHeader(int client_fd, ssize_t bytes_read, MessageHeader& header)
{
    if (!Protocol::parseHeader(recv_buffer, bytes_read, header))
        return false;

    uint32_t payload_size = Protocol::getPayloadSize(header);

    if (bytes_read < sizeof(MessageHeader) + payload_size)
        return false;

    return true;
}

void Server::handleConnectMessage(int client_fd)
{
    DEBUG_LOG("Client " + std::to_string(client_fd) + " sent connect message");
}

void Server::handlePlayerInputMessage(int client_fd, const MessageHeader &header)
{
    uint32_t payload_size = Protocol::getPayloadSize(header);

    if (payload_size >= 1) {
        bool jet_activated = (recv_buffer[sizeof(MessageHeader)] != 0);
        handlePlayerInput(client_fd, jet_activated);

        logPlayerInput(client_fd, jet_activated);
    }
}

void Server::logPlayerInput(int client_fd, bool jet_activated)
{
    auto player_it = players.find(client_fd);
    if (player_it != players.end()) {
        DEBUG_LOG("Player " + std::to_string(player_it->second->getPlayerNumber()) +
                 " input processed: jet " + (jet_activated ? "ON" : "OFF"));
    }
}

void Server::processClientMessage(int client_fd, ssize_t bytes_read)
{
    MessageHeader header;
    if (!parseMessageHeader(client_fd, bytes_read, header)) {
        return;
    }

    switch (header.type) {
        case MSG_CONNECT:
            handleConnectMessage(client_fd);
            break;

        case MSG_PLAYER_INPUT:
            handlePlayerInputMessage(client_fd, header);
            break;

        default:
            break;
    }
}

//=============================================================================
// Network Communication
//=============================================================================

void Server::sendToClient(int client_fd, const std::vector<uint8_t> &data)
{
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
     DEBUG_LOG("Broadcasting message to " + std::to_string(players.size()) + " clients");

     for (auto& pair : players) {
         int client_fd = pair.first;
         ssize_t sent = send(client_fd, data.data(), data.size(), 0);

         if (sent < 0) {
             DEBUG_LOG("Failed to broadcast to client: " + std::to_string(client_fd) +
                       ", errno=" + std::to_string(errno));
         } else {
             DEBUG_LOG("Successfully broadcast " + std::to_string(sent) +
                       " bytes to client: " + std::to_string(client_fd));
         }
     }
 }
