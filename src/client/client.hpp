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
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "../common/map.hpp"
#include "../common/protocol.hpp"


class GameState;
class Renderer;
class InputManager;

class Client {

private:
    int client_fd;
    std::string server_ip;
    int server_port;
    bool debug_mode;

    Map game_map;
    std::atomic<bool> game_started;
    std::atomic<bool> game_over;
    std::atomic<bool> connected;

    int my_player_number;

    std::thread network_thread;
    std::atomic<bool> running;

    static const size_t BUFFER_SIZE = 4096;
    char recv_buffer[BUFFER_SIZE];

    std::mutex data_mutex;
    std::condition_variable data_cv;

    std::queue<std::vector<uint8_t>> message_queue;
    std::mutex queue_mutex;

    std::chrono::steady_clock::time_point game_end_time;

    GameState *game_state;

    bool connectToServer();
    struct sockaddr_in setupServerAddress();
    void setSocketNonBlocking();
    void sendConnectMessage();

    void networkLoop();
    void processOutgoingMessages();
    void readIncomingData();
    void processMessage(const MessageHeader& header, const char *data, size_t data_size);
    void sendToServer(const std::vector<uint8_t> &data);

    void handleGameStart();
    void handleMapData(const char *data, size_t data_size);
    void handleGameState(const char *data, size_t data_size);
    void handleCollision(const char *data, size_t data_size);
    void handleGameEnd(const char *data, size_t data_size);

public:
    Client(const std::string &server_ip, int server_port, bool debug_mode);
    ~Client();

    bool initialize();
    void run(InputManager &input_manager, Renderer& renderer);
    void forceGameStarted() { game_started = true; }
    void stop();

    void sendPlayerInput(bool jet_activated);

    bool isConnected() const { return connected; }
    bool isGameStarted() const { return game_started; }
    bool isGameOver() const { return game_over; }
    const Map &getMap() const { return game_map; }
    int getPlayerNumber() const { return my_player_number; }

    void setGameState(GameState *state) { game_state = state; }
    GameState *getGameState() const { return game_state; }
};

#endif
