// Server core: listening socket, clients, message handling, fixed 60 Hz loop

#include "server.hpp"
#include "../common/debug.hpp"
#include "../common/socket.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

//=============================================================================
// Constructor & Destructor
//=============================================================================

Server::Server(const ServerConfig &config)
    : config(config), required_players(config.solo ? 1 : 2) {
    g_logger.setDebugMode(config.debug_mode);
}

Server::~Server()
{
    for (const Socket::PollEntry &entry : poll_fds)
        Socket::close(entry.fd);
}

//=============================================================================
// Server Initialization
//=============================================================================

// A solo run is nobody else's business: it only listens on loopback
bool Server::initialize()
{
    if (!Socket::startup() || !resetMap())
        return false;

    server_fd = Socket::listen(config.port, config.solo);
    if (server_fd < 0) {
        std::cerr << "Cannot listen on port " << config.port << ": " << Socket::lastError() << std::endl;
        return false;
    }

    poll_fds.push_back({server_fd});
    announce("Samuride server listening on port " + std::to_string(config.port) + (config.solo ? " (solo)" : ""));
    return true;
}

void Server::announce(const std::string &message) const
{
    if (!config.quiet)
        std::cout << message << std::endl;
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

    while (!stopping) {
        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(next_tick - clock::now());

        if (!Socket::poll(poll_fds, std::max<int>(wait.count(), 0)))
            break;

        processSocketEvents();

        while (clock::now() >= next_tick) {
            updateGameState();
            next_tick += tick;
        }
        flushClients();
    }
}

void Server::processSocketEvents()
{
    // Copy: accepting or removing a client changes poll_fds
    std::vector<Socket::PollEntry> entries = poll_fds;

    for (const Socket::PollEntry &entry : entries) {
        if (entry.fd == server_fd && entry.readable) {
            acceptNewClient();
        } else if (entry.readable) {
            handleClientData(entry.fd);
        } else if (entry.closed) {
            removeClient(entry.fd);
        }
    }
}

//=============================================================================
// Client Connection Management
//=============================================================================

void Server::acceptNewClient()
{
    int client_fd = Socket::accept(server_fd);

    if (client_fd < 0)
        return;

    if (phase != Phase::Waiting || players.size() >= required_players) {
        DEBUG_LOG("Lobby full, rejecting client: " + std::to_string(client_fd));
        Socket::close(client_fd);
        return;
    }

    poll_fds.push_back({client_fd});
    players.try_emplace(client_fd, client_fd);

    announce("Player connected (" + std::to_string(players.size()) + "/" + std::to_string(required_players) + ")");
}

void Server::removeClient(int client_fd)
{
    if (players.erase(client_fd) == 0)
        return;

    ready_clients.erase(client_fd);
    Socket::close(client_fd);
    std::erase_if(poll_fds, [client_fd](const Socket::PollEntry &entry) { return entry.fd == client_fd; });

    announce("Player disconnected (" + std::to_string(players.size()) + "/" + std::to_string(required_players) + ")");

    handlePlayerDisconnection();
}

void Server::handlePlayerDisconnection()
{
    bool in_game = phase == Phase::Countdown || phase == Phase::Running;

    if (in_game)
        endGame(players.size() == 1 && !config.solo ? players.begin()->second.getPlayerNumber() : -1);
    if (phase == Phase::Over)
        reopenLobby();
}

// Someone left: no rematch can come, whoever stays waits for a new choom (the map goes out at the start)
void Server::reopenLobby()
{
    resetMap();
    ready_clients.clear();
    phase = Phase::Waiting;
    announce("Lobby open");
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
            handleConnect(client_fd, payload);
            break;

        case MSG_PLAYER_INPUT:
            if (!payload.empty())
                handlePlayerInput(client_fd, payload[0] != 0);
            break;

        case MSG_READY:
            handleReady(client_fd);
            break;

        case MSG_ATTACK:
            // The key going down (1, or empty); coming back up (0) changes nothing
            if (payload.empty() || payload[0] != 0)
                handleAttack(client_fd);
            break;

        case MSG_ULTIMATE:
            handleUltimate(client_fd);
            break;

        default:
            break;
    }
}

// Payload: the runner picked, the district and the mode (NOT_PICKED when joining: the host picks),
// then the upgrade levels (see docs/PROTOCOL.md). District and mode are the first player's call: the
// host, who launched the server. Only heard in the lobby: sent again during a run, it would rebuild
// the map under the players.
void Server::handleConnect(int client_fd, const std::vector<uint8_t> &payload)
{
    DEBUG_LOG("Client " + std::to_string(client_fd) + " sent connect message");

    if (phase != Phase::Waiting)
        return;
    Player &player = players.at(client_fd);
    player.setCharacter(payload.empty() ? 0 : payload[0]);

    // The upgrade levels, each kept within what the shop sells
    UpgradeLevels levels{};
    for (int i = 0; i < UPGRADE_COUNT && 3 + i < static_cast<int>(payload.size()); i++)
        levels[i] = std::min<int>(payload[3 + i], UPGRADES[i].max_level);
    player.setUpgrades(levels);

    if (players.size() != 1)
        return;
    if (payload.size() >= 2 && payload[1] != NOT_PICKED)
        district = payload[1];

    // CYBERPSYCHO needs a map with drones
    GameMode wanted = payload.size() >= 3 && payload[2] == 1 ? GameMode::Cyberpsycho : GameMode::Classic;
    if (wanted != mode) {
        mode = wanted;
        resetMap();
    }
}

//=============================================================================
//=============================================================================

// Queued, then sent as far as the socket takes it: a packet cut in half would scramble the stream
void Server::sendToClient(int client_fd, const std::vector<uint8_t> &data)
{
    auto it = players.find(client_fd);

    if (it == players.end())
        return;

    std::vector<uint8_t> &out = it->second.getSendBuffer();
    out.insert(out.end(), data.begin(), data.end());
    flush(it->second);
}

void Server::flush(Player &player)
{
    std::vector<uint8_t> &out = player.getSendBuffer();
    int sent = out.empty() ? 0 : Socket::send(player.getClientFd(), out);

    if (sent > 0)
        out.erase(out.begin(), out.begin() + sent);
}

// Every tick: what the sockets couldn't take earlier goes out, and clients too far behind are dropped
void Server::flushClients()
{
    std::vector<int> lagging;

    for (auto &[client_fd, player] : players) {
        flush(player);
        if (player.getSendBuffer().size() > MAX_SEND_BACKLOG)
            lagging.push_back(client_fd);
    }
    for (int client_fd : lagging) {
        DEBUG_LOG("Client " + std::to_string(client_fd) + " can't keep up, dropping");
        removeClient(client_fd);
    }
}

void Server::broadcastToAllClients(const std::vector<uint8_t>& data)
{
    for (auto &pair : players)
        sendToClient(pair.first, data);
}
