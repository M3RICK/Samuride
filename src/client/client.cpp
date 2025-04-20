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

Client::Client(const std::string& server_ip, int server_port, bool debug_mode)
    : client_fd(-1), server_ip(server_ip), server_port(server_port), debug_mode(debug_mode),
      game_started(false), game_over(false), connected(false), my_player_number(-1),
      running(false), game_state(nullptr) {
    g_logger.setDebugMode(debug_mode);
}

Client::~Client()
{
    stop();

    if (client_fd >= 0) {
        close(client_fd);
    }
}

bool Client::initialize()
{
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return false;
    }
    return connectToServer();
}

bool Client::connectToServer()
{
    struct sockaddr_in server_addr = setupServerAddress();

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        return false;
    }

    DEBUG_LOG("Connected to server: " + server_ip + ":" + std::to_string(server_port));

    setSocketNonBlocking();
    sendConnectMessage();

    connected = true;
    return true;
}

struct sockaddr_in Client::setupServerAddress()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << server_ip << std::endl;
    }
    return server_addr;
}

void Client::setSocketNonBlocking()
{
    int flags = fcntl(client_fd, F_GETFL, 0);

    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
}

void Client::sendConnectMessage()
{
    std::vector<uint8_t> payload;
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_CONNECT, payload);

    sendToServer(packet);
}

void Client::run(InputManager &input, Renderer &renderer)
{
    running = true;
    network_thread = std::thread(&Client::networkLoop, this);

    while (running) {
        if (game_started.load(std::memory_order_acquire)) {
            DEBUG_LOG("Game is started in main loop: " + std::to_string(game_started.load()));
        }

        input.processInputs();
        renderer.render();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (game_over) {
            if (input.shouldExit()) {
                running = false;
            }
        }
    }

    if (network_thread.joinable()) {
        network_thread.join();
    }
}

void Client::stop()
{
    running = false;

    if (network_thread.joinable())
        network_thread.join();
}

void Client::networkLoop()
{
    int cycles = 0;

    while (running) {
        processOutgoingMessages();
        readIncomingData();
        cycles++;

        if (cycles > 100 && my_player_number >= 0 && !game_started)
            game_started = true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


void Client::processOutgoingMessages()
{
    std::lock_guard<std::mutex> lock(queue_mutex);

    while (!message_queue.empty()) {
        const auto& message = message_queue.front();
        send(client_fd, message.data(), message.size(), 0);

        DEBUG_PACKET_SEND(reinterpret_cast<const char*>(message.data()), message.size());

        message_queue.pop();
    }
}

void Client::readIncomingData()
{
    memset(recv_buffer, 0, BUFFER_SIZE);
    ssize_t bytes_read = recv(client_fd, recv_buffer, BUFFER_SIZE, 0);

    if (bytes_read > 0) {
        DEBUG_PACKET_RECV(recv_buffer, bytes_read);

        MessageHeader header;

        if (Protocol::parseHeader(recv_buffer, bytes_read, header)) {
            if (header.type == MSG_GAME_START) {
                handleGameStart();
                return;
            }
            processMessage(header, recv_buffer + sizeof(MessageHeader),
                          bytes_read - sizeof(MessageHeader));
        }
    } else if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        connected = false;
        running = false;
    }
}


void Client::processMessage(const MessageHeader &header, const char *data, size_t data_size)
{
    uint32_t payload_size = Protocol::getPayloadSize(header);

    if (header.type == MSG_GAME_START) {
        DEBUG_LOG("*** RECEIVED GAME START MESSAGE, SETTING GAME STARTED FLAG ***");
        game_started = true;
        return;
    }

    switch (header.type) {
        case MSG_MAP_DATA:
            handleMapData(data, payload_size);
            break;
        case MSG_GAME_STATE:
            handleGameState(data, payload_size);
            break;
        case MSG_COLLISION:
            handleCollision(data, payload_size);
            break;
        case MSG_GAME_END:
            handleGameEnd(data, payload_size);
            break;
        case MSG_COUNTDOWN:
            if (payload_size >= 1) {
                int count = data[0];
                if (count > 0) {
                    DEBUG_LOG("Game starting in " + std::to_string(count) + "...");
                } else {
                    DEBUG_LOG("BURN THE CITY!");
                }
            }
            break;
        default:
            break;
    }
}

void Client::handleMapData(const char *data, size_t size)
{
    DEBUG_LOG("Received map data");
    std::vector<uint8_t> map_data(data, data + size);

    if (game_map.loadFromData(map_data)) {
        DEBUG_LOG("Map loaded successfully: " + std::to_string(game_map.getWidth()) +
                  "x" + std::to_string(game_map.getHeight()));
    } else {
        DEBUG_LOG("Failed to load map data");
    }
}

void Client::handleGameStart()
{
    DEBUG_LOG("Game start received, beginning countdown");

    std::thread([this]() {
        for (int i = 3; i > 0; i--) {
            DEBUG_LOG("Game starting in " + std::to_string(i) + "...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        DEBUG_LOG("Game started!");
        game_started = true;
    }).detach();
}

void Client::handleGameState(const char *data, size_t size)
{
    if (!game_state)
        return;

    std::lock_guard<std::mutex> lock(data_mutex);

    size_t pos = 0;

    while (pos + 8 <= size) {
        int player_number = data[pos++];
        uint16_t x = (data[pos] << 8) | data[pos+1]; pos += 2;
        uint16_t y = (data[pos] << 8) | data[pos+1]; pos += 2;
        uint16_t score = (data[pos] << 8) | data[pos+1]; pos += 2;
        bool jet_active = data[pos++] != 0;

        if (my_player_number == -1) {
            my_player_number = player_number;
            DEBUG_LOG("Setting my player number to: " + std::to_string(my_player_number));
        }

        game_state->updatePlayer(player_number, x, y, score, jet_active);

        std::string isMe = (player_number == my_player_number) ? " (ME)" : "";
        DEBUG_LOG("Updated player " + std::to_string(player_number) + isMe +
                  ": pos=(" + std::to_string(x) + "," + std::to_string(y) +
                  "), jet=" + (jet_active ? "ON" : "OFF"));
    }
}

void Client::handleCollision(const char *data, size_t size)
{
    if (size < 5)
        return;

    char collision_type = data[0];
    uint16_t x = (data[1] << 8) | data[2];
    uint16_t y = (data[3] << 8) | data[4];

    DEBUG_LOG("Collision: type=" + std::string(1, collision_type) +
              ", position=(" + std::to_string(x) + "," + std::to_string(y) + ")");

    if (game_state)
        game_state->handleCollision(collision_type, x, y);
}

void Client::handleGameEnd(const char* data, size_t size)
{
    game_over = true;

    if (size >= 1) {

        int winner = static_cast<uint8_t>(data[0]);

        if (winner != 0xFF) {
            DEBUG_LOG("Game over. Player " + std::to_string(winner) + " wins!");
            if (game_state)
                game_state->setWinner(winner);
        } else {
            DEBUG_LOG("Game over. No winner.");
            if (game_state)
                game_state->setWinner(-1);
        }
    }
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
    if (!connected || !game_started || game_over)
        return;

    std::vector<uint8_t> payload = { static_cast<uint8_t>(jet_activated ? 1 : 0) };
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_PLAYER_INPUT, payload);

    DEBUG_LOG("Sending input: jet " + std::string(jet_activated ? "ON" : "OFF"));
    sendToServer(packet);
}
