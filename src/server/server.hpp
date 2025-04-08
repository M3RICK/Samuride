#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
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

    bool initializeServer();
    void acceptNewClient();
    void handleClientData(int client_fd);
    void sendToClient(int client_fd, const std::vector<uint8_t>& data);
    void broadcastToAllClients(const std::vector<uint8_t>& data);
    void removeClient(int client_fd);
    void startGame();
    void checkGameState();

public:
    Server(int port, const std::string& map_path, bool debug_mode);
    ~Server();

    bool initialize();
    void run();
    Map& getMap() { return game_map; }

    void handlePlayerInput(int client_fd, bool jet_activated);
    void notifyCollision(int client_fd, char collision_type, int x, int y);
    void endGame(int winner_fd);
};

#endif // SERVER_HPP
