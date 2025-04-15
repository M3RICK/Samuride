#include "client.hpp"
#include "state.hpp"
#include "render.hpp"
#include "inputs.hpp"
#include "../common/debug.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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
    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Connect to server
    return connectToServer();
}

bool Client::connectToServer()
{
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << server_ip << std::endl;
        return false;
    }

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return false;
    }

    DEBUG_LOG("Connected to server: " + server_ip + ":" + std::to_string(server_port));

    // Set socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // Send connect message
    std::vector<uint8_t> payload;
    std::vector<uint8_t> packet = Protocol::createPacket(MSG_CONNECT, payload);
    sendToServer(packet);

    connected = true;
    return true;
}

void Client::run(InputManager& input_manager, Renderer& renderer)
{
    // Start network thread
    running = true;
    network_thread = std::thread(&Client::networkLoop, this);

    // Main game loop
    while (running) {
        // Handle inputs
        input_manager.processInputs();

        // Render game
        renderer.render();

        // Small delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Check if game is over
        if (game_over) {
            // Wait for player to acknowledge game over
            if (input_manager.shouldExit()) {
                running = false;
            }
            continue;
        }
    }

    // Wait for network thread to terminate
    if (network_thread.joinable()) {
        network_thread.join();
    }
}

void Client::stop() {
    running = false;
    if (network_thread.joinable()) {
        network_thread.join();
    }
}

void Client::networkLoop()
{
    while (running) {
        // Process outgoing messages
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            while (!message_queue.empty()) {
                const auto& message = message_queue.front();
                send(client_fd, message.data(), message.size(), 0);
                DEBUG_PACKET_SEND(reinterpret_cast<const char*>(message.data()), message.size());
                message_queue.pop();
            }
        }

        // Read incoming data
        memset(recv_buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = recv(client_fd, recv_buffer, BUFFER_SIZE, 0);

        if (bytes_read > 0) {
            DEBUG_PACKET_RECV(recv_buffer, bytes_read);

            // Process message
            MessageHeader header;
            if (Protocol::parseHeader(recv_buffer, bytes_read, header)) {
                processMessage(header, recv_buffer + sizeof(MessageHeader),
                              bytes_read - sizeof(MessageHeader));
            }
        } else if (bytes_read == 0) {
            // Server disconnected
            DEBUG_LOG("Server disconnected");
            connected = false;
            running = false;
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Error
            DEBUG_LOG("Error reading from server: " + std::to_string(errno));
            connected = false;
            running = false;
            break;
        }

        // Small delay to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Client::processMessage(const MessageHeader& header, const char* data, size_t data_size)
{
    uint32_t payload_size = Protocol::getPayloadSize(header);

    // Verify we have the complete payload
    if (data_size < payload_size) {
        DEBUG_LOG("Incomplete message received");
        return;
    }

    // Process different message types
    switch (header.type) {
        case MSG_MAP_DATA: {
            DEBUG_LOG("Received map data");
            std::vector<uint8_t> map_data(data, data + payload_size);
            if (game_map.loadFromData(map_data)) {
                DEBUG_LOG("Map loaded successfully: " + std::to_string(game_map.getWidth()) +
                          "x" + std::to_string(game_map.getHeight()));
            } else {
                DEBUG_LOG("Failed to load map data");
            }
            break;
        }

        case MSG_GAME_START: {
            DEBUG_LOG("Game started");
            game_started = true;
            //game_over = false; Restart a chaque fois
            break;
        }

        case MSG_GAME_STATE: {
            if (game_state) {
                // Lock the game state for updating
                std::lock_guard<std::mutex> lock(data_mutex);
                size_t pos = 0;

                while (pos + 8 <= payload_size) {
                    // Extract player data
                    int player_number = data[pos++];

                    uint16_t x = (static_cast<uint8_t>(data[pos]) << 8) |
                                 static_cast<uint8_t>(data[pos+1]);
                    pos += 2;

                    uint16_t y = (static_cast<uint8_t>(data[pos]) << 8) |
                                 static_cast<uint8_t>(data[pos+1]);
                    pos += 2;

                    uint16_t score = (static_cast<uint8_t>(data[pos]) << 8) |
                                     static_cast<uint8_t>(data[pos+1]);
                    pos += 2;

                    bool jet_active = data[pos++] != 0;

                    // Update player in game state
                    game_state->updatePlayer(player_number, x, y, score, jet_active);

                    // If this is our player, remember our player number
                    if (my_player_number == -1) {
                        my_player_number = player_number;
                    }
                }
            }
            break;
        }

        case MSG_COLLISION: {
            if (payload_size >= 5) {
                char collision_type = data[0];

                uint16_t x = (static_cast<uint8_t>(data[1]) << 8) |
                             static_cast<uint8_t>(data[2]);

                uint16_t y = (static_cast<uint8_t>(data[3]) << 8) |
                             static_cast<uint8_t>(data[4]);

                DEBUG_LOG("Collision: type=" + std::string(1, collision_type) +
                          ", position=(" + std::to_string(x) + "," + std::to_string(y) + ")");

                if (game_state) {
                    game_state->handleCollision(collision_type, x, y);
                }
            }
            break;
        }

        case MSG_GAME_END: {
            game_over = true;

            if (payload_size >= 1) {
                int winner = static_cast<uint8_t>(data[0]);
                if (winner != 0xFF) {
                    DEBUG_LOG("Game over. Player " + std::to_string(winner) + " wins!");
                    if (game_state) {
                        game_state->setWinner(winner);
                    }
                } else {
                    DEBUG_LOG("Game over. No winner.");
                    if (game_state) {
                        game_state->setWinner(-1);
                    }
                }
            }
            break;
        }

        default:
            DEBUG_LOG("Unknown message type: " + std::to_string(header.type));
            break;
    }
}

void Client::sendToServer(const std::vector<uint8_t>& data)
{
    if (!connected || data.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(queue_mutex);
    message_queue.push(data);
}

void Client::sendPlayerInput(bool jet_activated)
{
    if (!connected || !game_started || game_over) {
        return;
    }

    std::vector<uint8_t> payload;
    payload.push_back(jet_activated ? 1 : 0);

    std::vector<uint8_t> packet = Protocol::createPacket(MSG_PLAYER_INPUT, payload);
    sendToServer(packet);
}
