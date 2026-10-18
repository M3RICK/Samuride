/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "client.hpp"
#include "state.hpp"
#include "render.hpp"
#include "inputs.hpp"
#include "../common/debug.hpp"
#include "../common/socket.hpp"

#include <iostream>
#include <map>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

Client::Client(const std::string& server_ip, int server_port, int character, int district,
               bool debug_mode, GameState &game_state)
    : client_fd(-1), server_ip(server_ip), server_port(server_port), character(character), district(district),
      game_state(game_state), connected(false), running(false) {
    g_logger.setDebugMode(debug_mode);
}

Client::~Client()
{
    stop();

    if (client_fd >= 0) {
        close(client_fd);
    }
}

// Retries for a couple of seconds: a server we just launched may not be listening yet
bool Client::initialize()
{
    for (int attempt = 0; attempt < 40; attempt++) {
        if (connectToServer())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

bool Client::connectToServer()
{
    addrinfo hints = {};
    addrinfo *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_ip.c_str(), std::to_string(server_port).c_str(), &hints, &result) != 0)
        return false;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    bool ok = client_fd >= 0 && connect(client_fd, result->ai_addr, result->ai_addrlen) == 0;
    freeaddrinfo(result);

    if (!ok) {
        if (client_fd >= 0)
            close(client_fd);
        client_fd = -1;
        return false;
    }

    DEBUG_LOG("Connected to server: " + server_ip + ":" + std::to_string(server_port));

    Socket::setNonBlocking(client_fd);
    connected = true;
    sendConnectMessage();
    return true;
}

void Client::sendConnectMessage()
{
    std::vector<uint8_t> payload = { static_cast<uint8_t>(character) };

    // No district when joining: the host's applies
    if (district >= 0)
        payload.push_back(district);
    sendToServer(Protocol::createPacket(MSG_CONNECT, payload));
}

Client::RunResult Client::run(InputManager &input, Renderer &renderer)
{
    using clock = std::chrono::steady_clock;
    Phase last_phase = Phase::Waiting;
    auto phase_since = clock::now();

    running = true;
    network_thread = std::thread(&Client::networkLoop, this);

    while (true) {
        input.processInputs();
        if (input.closeRequested() || input.backRequested())
            break;

        Phase phase = game_state.getPhase();
        if (phase != last_phase) {
            last_phase = phase;
            phase_since = clock::now();
        }

        // Short grace period so a jetpack still held at death doesn't skip the results
        bool can_rematch = phase == Phase::Over && clock::now() - phase_since > std::chrono::milliseconds(1500);
        if (input.confirmPressed() && can_rematch && game_state.requestRematch())
            sendToServer(Protocol::createPacket(MSG_READY, {}));

        renderer.render(game_state, connected);
    }

    stop();
    return input.closeRequested() ? RunResult::Quit : RunResult::Menu;
}

void Client::stop()
{
    running = false;

    if (network_thread.joinable())
        network_thread.join();
}

void Client::networkLoop()
{
    pollfd pfd = {client_fd, POLLIN, 0};

    while (running && connected) {
        processOutgoingMessages();

        if (poll(&pfd, 1, 5) > 0)
            readIncomingData();
    }
}

void Client::processOutgoingMessages()
{
    std::lock_guard<std::mutex> lock(queue_mutex);

    while (!message_queue.empty()) {
        Socket::send(client_fd, message_queue.front());
        message_queue.pop();
    }
}

void Client::readIncomingData()
{
    if (!Socket::receive(client_fd, stream)) {
        DEBUG_LOG("Lost connection to server");
        connected = false;
        return;
    }

    MessageHeader header;
    std::vector<uint8_t> payload;

    while (Protocol::extractPacket(stream, header, payload))
        processMessage(header, payload);
}

void Client::processMessage(const MessageHeader &header, const std::vector<uint8_t> &payload)
{
    switch (header.type) {
        case MSG_MAP_DATA:
            game_state.pushMapData(payload);
            break;
        case MSG_GAME_START:
            handleGameStart(payload);
            break;
        case MSG_COUNTDOWN:
            if (!payload.empty())
                game_state.setCountdown(payload[0]);
            break;
        case MSG_GAME_STATE:
            handleGameState(payload);
            break;
        case MSG_COLLISION:
            handleCollision(payload);
            break;
        case MSG_GAME_END:
            handleGameEnd(payload);
            break;
        default:
            break;
    }
}

// [my number][player count]([number][character])...[district]
void Client::handleGameStart(const std::vector<uint8_t> &payload)
{
    std::map<int, int> roster;

    if (payload.size() < 2)
        return;

    size_t count = payload[1];
    for (size_t i = 0; i < count && 3 + 2 * i < payload.size(); i++)
        roster[payload[2 + 2 * i]] = payload[3 + 2 * i];

    size_t district_pos = 2 + 2 * count;
    game_state.startCountdown(payload[0], roster, district_pos < payload.size() ? payload[district_pos] : 0);
}

void Client::handleGameState(const std::vector<uint8_t> &payload)
{
    for (size_t pos = 0; pos + 10 <= payload.size(); pos += 10) {
        const uint8_t *p = payload.data() + pos;

        game_state.updatePlayer(p[0],
            Protocol::readU32(p + 1) / POSITION_SCALE,
            Protocol::readU16(p + 5) / POSITION_SCALE,
            Protocol::readU16(p + 7),
            p[9] != 0);
    }
}

void Client::handleCollision(const std::vector<uint8_t> &payload)
{
    if (payload.size() < 8)
        return;

    game_state.handleCollision(payload[0], payload[1],
        Protocol::readU32(payload.data() + 2), Protocol::readU16(payload.data() + 6));
}

void Client::handleGameEnd(const std::vector<uint8_t> &payload)
{
    if (payload.empty())
        return;

    int winner = payload[0] == 0xFF ? -1 : payload[0];

    DEBUG_LOG(winner >= 0 ? "Game over. Player " + std::to_string(winner) + " wins!" : "Game over. No winner.");
    game_state.setWinner(winner);
}

void Client::sendToServer(const std::vector<uint8_t>& data)
{
    if (!connected || data.empty())
        return;

    std::lock_guard<std::mutex> lock(queue_mutex);

    message_queue.push(data);
}

void Client::sendPlayerInput(bool jet_activated)
{
    std::vector<uint8_t> payload = { static_cast<uint8_t>(jet_activated ? 1 : 0) };

    sendToServer(Protocol::createPacket(MSG_PLAYER_INPUT, payload));
}
