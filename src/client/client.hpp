#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include "../common/map.hpp"
#include "../common/protocol.hpp"

// Forward declarations
class GameState;
class Renderer;
class InputManager;

class Client {
private:
    int client_fd;
    std::string server_ip;
    int server_port;
    bool debug_mode;

    // Game data
    Map game_map;
    std::atomic<bool> game_started;
    std::atomic<bool> game_over;
    std::atomic<bool> connected;

    // Player data
    int my_player_number;

    // Communication
    std::thread network_thread;
    std::atomic<bool> running;

    // Buffer for receiving data
    static const size_t BUFFER_SIZE = 4096;
    char recv_buffer[BUFFER_SIZE];

    // Thread synchronization
    std::mutex data_mutex;
    std::condition_variable data_cv;

    // Message queue
    std::queue<std::vector<uint8_t>> message_queue;
    std::mutex queue_mutex;

    // Pause at end of game
    std::chrono::steady_clock::time_point game_end_time;

    // Game state reference
    GameState* game_state;

    // Methods
    bool connectToServer();
    void networkLoop();
    void processMessage(const MessageHeader& header, const char* data, size_t data_size);
    void sendToServer(const std::vector<uint8_t>& data);

public:
    Client(const std::string& server_ip, int server_port, bool debug_mode);
    ~Client();

    bool initialize();
    void run(InputManager& input_manager, Renderer& renderer);
    void stop();

    // Game control methods
    void sendPlayerInput(bool jet_activated);

    // Getters
    bool isConnected() const { return connected; }
    bool isGameStarted() const { return game_started; }
    bool isGameOver() const { return game_over; }
    const Map& getMap() const { return game_map; }
    int getPlayerNumber() const { return my_player_number; }

    // State access
    void setGameState(GameState* state) { game_state = state; }
    GameState* getGameState() const { return game_state; }
};

#endif // CLIENT_HPP
