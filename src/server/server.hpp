/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef SERVER_HPP
    #define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <poll.h>
#include "../common/map.hpp"
#include "../common/protocol.hpp"
#include "player.hpp"

class Server {
private:
    enum class Phase { Waiting, Countdown, Running, Over };

    static constexpr int TICK_RATE = 60;
    static constexpr float TICK_DT = 1.0f / TICK_RATE;
    static constexpr int COUNTDOWN_SECONDS = 3;

    // Endless run: the map is generated CHUNK_WIDTH columns at a time, LOOKAHEAD columns ahead of the leader
    static constexpr size_t MAP_HEIGHT = 10;
    static constexpr size_t CHUNK_WIDTH = 40;
    static constexpr size_t SAFE_START = 14;
    static constexpr float LOOKAHEAD = 40;
    static constexpr int CHUNKS_TO_MAX_DIFFICULTY = 12;

    // Run speed in tiles per second, ramps up over time
    static constexpr float START_SPEED = 7.0f;
    static constexpr float MAX_SPEED = 16.0f;
    static constexpr float ACCELERATION = 0.12f;

    int server_fd;
    int port;
    std::string map_path;
    bool solo;
    bool debug_mode;
    size_t required_players;

    Map game_map;
    std::mt19937 rng;
    int chunks_generated;
    float run_speed;

    Phase phase;
    int countdown;
    int countdown_ticks;
    std::set<int> ready_clients;
    uint8_t district;

    std::vector<pollfd> poll_fds;
    std::map<int, Player> players;

    static const size_t BUFFER_SIZE = 1024;
    uint8_t recv_buffer[BUFFER_SIZE];

    //===========================================================================
    // Server Initialization
    //===========================================================================

    bool initializeServer();

    bool resetMap();

    void extendMap(float leader_x);

    bool setSocketOptions();

    bool bindSocket();

    void setNonBlocking(int fd);

    //===========================================================================
    // Client Connection Management
    //===========================================================================

    void acceptNewClient();

    void sendMapToClient(int client_fd);

    void removeClient(int client_fd);

    void handlePlayerDisconnection();

    //===========================================================================
    // Client Data Handling
    //===========================================================================

    void handleClientData(int client_fd);

    void processClientMessage(int client_fd, const MessageHeader &header, const std::vector<uint8_t> &payload);

    //===========================================================================
    // Socket Event Processing
    //===========================================================================

    void processSocketEvents();

    void updateGameState();

    //===========================================================================
    // Network Communication
    //===========================================================================

    void sendToClient(int client_fd, const std::vector<uint8_t> &data);

    void broadcastToAllClients(const std::vector<uint8_t> &data);

    //===========================================================================
    // Game Logic
    //===========================================================================

    void startGame();

    void handleReady(int client_fd);

    void updateCountdown();

    void updatePlayers();

    bool checkPlayerCollisions(Player &player);

    void sendGameState();

    void handlePlayerInput(int client_fd, bool jet_activated);

    void notifyCollision(char collision_type, const Player &player, int x, int y);

    void endGame(int winner_number);

public:
    Server(int port, const std::string& map_path, bool solo, bool debug_mode);

    ~Server();

    bool initialize();

    void run();
};

#endif
