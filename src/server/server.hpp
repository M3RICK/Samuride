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
#include <thread>
#include <chrono>
#include <poll.h>
#include <netinet/in.h>
#include "../common/map.hpp"
#include "../common/protocol.hpp"

class Player;

class Server {
private:
    int server_fd;
    int port;
    std::string map_path;
    bool debug_mode;

    Map game_map;
    bool game_started;

    std::vector<pollfd> poll_fds;
    std::map<int, Player*> players;

    static const size_t BUFFER_SIZE = 1024;
    char recv_buffer[BUFFER_SIZE];

    //===========================================================================
    // Server Initialization
    //===========================================================================

    bool initializeServer();

    bool loadGameMap();

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

    void handleReceiveError(int client_fd, ssize_t bytes_read);

    bool parseMessageHeader(int client_fd, ssize_t bytes_read, MessageHeader &header);

    void handleConnectMessage(int client_fd);

    void handlePlayerInputMessage(int client_fd, const MessageHeader &header);

    void logPlayerInput(int client_fd, bool jet_activated);

    void processClientMessage(int client_fd, ssize_t bytes_read);

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

    void initializePlayerPositions();

    void checkGameState();

    void checkGameOverConditions();

    void constrainPlayerToMap(Player *player);

    bool checkPlayerCollisions(int client_fd, Player *player);

    void updateAndSendGameState();

    void addPlayerStateToPacket(std::vector<uint8_t> &data, Player *player);

public:
    Server(int port, const std::string& map_path, bool debug_mode);

    ~Server();

    bool initialize();

    void run();

    Map &getMap()
    {
        return game_map;
    }

    void handlePlayerInput(int client_fd, bool jet_activated);
    void notifyCollision(int client_fd, char collision_type, int x, int y);

    void startGame();

    void endGame(int winner_fd);
};

#endif
