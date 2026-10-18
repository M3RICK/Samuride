/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include "../common/debug.hpp"
#include "../common/socket.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

//=============================================================================
// Constructor & Destructor
//=============================================================================

Server::Server(int port, const std::string& map_path, bool solo, bool debug_mode)
    : server_fd(-1), port(port), map_path(map_path), solo(solo), debug_mode(debug_mode),
      required_players(solo ? 1 : 2), rng(std::random_device{}()), chunks_generated(0),
      run_speed(START_SPEED), phase(Phase::Waiting), countdown(0), countdown_ticks(0), district(0) {
    g_logger.setDebugMode(debug_mode);
}

Server::~Server()
{
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
    if (!resetMap())
        return false;

    return initializeServer();
}

bool Server::initializeServer()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    Socket::setNonBlocking(server_fd);

    if (!setSocketOptions() || !bindSocket() || listen(server_fd, 5) < 0) {
        std::cerr << "Cannot listen on port " << port << ": " << strerror(errno) << std::endl;
        close(server_fd);
        return false;
    }

    pollfd pfd = {server_fd, POLLIN, 0};
    poll_fds.push_back(pfd);

    std::cout << "Samuride server listening on port " << port << (solo ? " (solo)" : "") << std::endl;
    return true;
}

bool Server::setSocketOptions()
{
    int opt = 1;

    return setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

bool Server::bindSocket()
{
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    // A solo run is nobody else's business
    address.sin_addr.s_addr = htonl(solo ? INADDR_LOOPBACK : INADDR_ANY);
    address.sin_port = htons(port);

    return bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == 0;
}

//=============================================================================
// Main Server Loop
//=============================================================================

// Fixed tick: the game advances TICK_RATE times per second, whatever the network does
void Server::run()
{
    using clock = std::chrono::steady_clock;
    const auto tick = std::chrono::microseconds(1000000 / TICK_RATE);
    auto next_tick = clock::now();

    while (true) {
        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(next_tick - clock::now());

        if (poll(poll_fds.data(), poll_fds.size(), std::max<int>(wait.count(), 0)) < 0 && errno != EINTR)
            break;

        processSocketEvents();

        while (clock::now() >= next_tick) {
            updateGameState();
            next_tick += tick;
        }
    }
}

void Server::processSocketEvents()
{
    // Copy: accepting or removing a client changes poll_fds
    std::vector<pollfd> fds = poll_fds;

    for (const pollfd &pfd : fds) {
        if (pfd.revents == 0)
            continue;

        if (pfd.fd == server_fd) {
            acceptNewClient();
        } else if (pfd.revents & POLLIN) {
            handleClientData(pfd.fd);
        } else if (pfd.revents & (POLLHUP | POLLERR)) {
            removeClient(pfd.fd);
        }
    }
}

//=============================================================================
// Client Connection Management
//=============================================================================

void Server::acceptNewClient()
{
    int client_fd = accept(server_fd, nullptr, nullptr);

    if (client_fd < 0)
        return;

    if (phase != Phase::Waiting || players.size() >= required_players) {
        DEBUG_LOG("Lobby full, rejecting client: " + std::to_string(client_fd));
        close(client_fd);
        return;
    }

    Socket::setNonBlocking(client_fd);

    pollfd pfd = {client_fd, POLLIN, 0};
    poll_fds.push_back(pfd);
    players.try_emplace(client_fd, client_fd);

    std::cout << "Player connected (" << players.size() << "/" << required_players << ")" << std::endl;

    sendMapToClient(client_fd);
}

void Server::sendMapToClient(int client_fd)
{
    sendToClient(client_fd, Protocol::createPacket(MSG_MAP_DATA, game_map.serialize()));
}

void Server::removeClient(int client_fd)
{
    if (players.erase(client_fd) == 0)
        return;

    ready_clients.erase(client_fd);
    close(client_fd);
    std::erase_if(poll_fds, [client_fd](const pollfd &pfd) { return pfd.fd == client_fd; });

    std::cout << "Player disconnected (" << players.size() << "/" << required_players << ")" << std::endl;

    handlePlayerDisconnection();
}

void Server::handlePlayerDisconnection()
{
    bool in_game = phase == Phase::Countdown || phase == Phase::Running;

    if (in_game)
        endGame(players.size() == 1 && !solo ? players.begin()->second.getPlayerNumber() : -1);
}

//=============================================================================
// Client Data Handling
//=============================================================================

void Server::handleClientData(int client_fd)
{
    auto it = players.find(client_fd);

    if (it == players.end())
        return;

    std::vector<uint8_t> &stream = it->second.getRecvBuffer();

    if (!Socket::receive(client_fd, stream)) {
        removeClient(client_fd);
        return;
    }

    MessageHeader header;
    std::vector<uint8_t> payload;

    while (Protocol::extractPacket(stream, header, payload))
        processClientMessage(client_fd, header, payload);

    // Clients only send tiny packets, anything this big is garbage
    if (stream.size() > MAX_PENDING_BYTES) {
        DEBUG_LOG("Client " + std::to_string(client_fd) + " sent garbage, kicking");
        removeClient(client_fd);
    }
}

void Server::processClientMessage(int client_fd, const MessageHeader &header, const std::vector<uint8_t> &payload)
{
    switch (header.type) {
        case MSG_CONNECT:
            // Optional payload: the character the player picked, then the district.
            // The district is the first player's call: the host, who launched the server.
            if (!payload.empty())
                players.at(client_fd).setCharacter(payload[0]);
            if (payload.size() >= 2 && players.size() == 1)
                district = payload[1];
            DEBUG_LOG("Client " + std::to_string(client_fd) + " sent connect message");
            break;

        case MSG_PLAYER_INPUT:
            if (!payload.empty())
                handlePlayerInput(client_fd, payload[0] != 0);
            break;

        case MSG_READY:
            handleReady(client_fd);
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
    Socket::send(client_fd, data);
}

void Server::broadcastToAllClients(const std::vector<uint8_t>& data)
{
    for (auto &pair : players)
        sendToClient(pair.first, data);
}
