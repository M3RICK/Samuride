/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef CLIENT_HPP
    #define CLIENT_HPP

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

#include "../common/protocol.hpp"

class GameState;

// What the player brings to the server. District and mode: -1 when joining, the host picks.
struct Loadout {
    int character = 0;
    int district = -1;
    int mode = -1;
};

class Renderer;
class InputManager;

class Client {

private:
    int client_fd;
    std::string server_ip;
    int server_port;
    Loadout loadout;

    GameState &game_state;
    std::atomic<bool> connected;

    std::thread network_thread;
    std::atomic<bool> running;

    std::vector<uint8_t> stream;    // bytes received, not yet a full packet

    std::queue<std::vector<uint8_t>> message_queue;
    std::mutex queue_mutex;

    bool connectToServer();
    void sendConnectMessage();

    void networkLoop();
    void processOutgoingMessages();
    void readIncomingData();
    void processMessage(const MessageHeader &header, const std::vector<uint8_t> &payload);
    void sendToServer(const std::vector<uint8_t> &data);

    void handleGameStart(const std::vector<uint8_t> &payload);
    void handleShot(const std::vector<uint8_t> &payload);
    void handleGameState(const std::vector<uint8_t> &payload);
    void handleCollision(const std::vector<uint8_t> &payload);
    void handleGameEnd(const std::vector<uint8_t> &payload);

public:
    enum class RunResult { Menu, Quit };

    Client(const std::string &server_ip, int server_port, Loadout loadout, bool debug_mode, GameState &game_state);
    ~Client();

    bool initialize();
    RunResult run(InputManager &input_manager, Renderer &renderer);
    void stop();

    void sendPlayerInput(bool jet_activated);
    void sendAttack();

    bool isConnected() const { return connected; }
};

#endif
