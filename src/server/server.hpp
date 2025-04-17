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

    //===========================================================================
    // Server Initialization
    //===========================================================================

    // Initialize server socket and bind to port
    bool initializeServer();

    // Load the game map from file
    bool loadGameMap();

    // Set socket options like address reuse
    bool setSocketOptions();

    // Bind the server socket to address and port
    bool bindSocket();

    // Set a socket to non-blocking mode
    void setNonBlocking(int fd);

    //===========================================================================
    // Client Connection Management
    //===========================================================================

    // Accept a new client connection
    void acceptNewClient();

    // Send map data to a newly connected client
    void sendMapToClient(int client_fd);

    // Remove a client from the server
    void removeClient(int client_fd);

    // Handle game state changes when a player disconnects
    void handlePlayerDisconnection();

    //===========================================================================
    // Client Data Handling
    //===========================================================================

    // Handle data received from a client
    void handleClientData(int client_fd);

    // Handle receive errors or client disconnection
    void handleReceiveError(int client_fd, ssize_t bytes_read);

    // Process a message received from a client
    void processClientMessage(int client_fd, ssize_t bytes_read);

    //===========================================================================
    // Socket Event Processing
    //===========================================================================

    // Process events on all sockets
    void processSocketEvents();

    // Update game state if running or check if game should start
    void updateGameState();

    //===========================================================================
    // Network Communication
    //===========================================================================

    // Send data to a specific client
    void sendToClient(int client_fd, const std::vector<uint8_t>& data);

    // Broadcast data to all connected clients
    void broadcastToAllClients(const std::vector<uint8_t>& data);

    //===========================================================================
    // Game Logic
    //===========================================================================

    // Initialize positions for all players
    void initializePlayerPositions();

    // Check and update game state
    void checkGameState();

    // Check if the game is over due to win/lose conditions
    void checkGameOverConditions();

    // Constrain a player to stay within map boundaries
    void constrainPlayerToMap(Player* player);

    // Check for player collisions with map elements
    bool checkPlayerCollisions(int client_fd, Player* player);

    // Update and send game state to all clients
    void updateAndSendGameState();

    // Add a player's state to a network packet
    void addPlayerStateToPacket(std::vector<uint8_t>& data, Player* player);

public:
    /**
     * Constructor - initialize server with given parameters
     * @param port Port to listen on
     * @param map_path Path to the map file
     * @param debug_mode Enable debug output
     */
    Server(int port, const std::string& map_path, bool debug_mode);

    /**
     * Destructor - clean up resources
     */
    ~Server();

    /**
     * Initialize the server - load map and set up network
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize();

    /**
     * Run the main server loop
     */
    void run();

    /**
     * Get the game map
     * @return Reference to the game map
     */
    Map& getMap() { return game_map; }

    /**
     * Handle player input (jetpack activation)
     * @param client_fd Client socket
     * @param jet_activated Whether jetpack is active
     */
    void handlePlayerInput(int client_fd, bool jet_activated);

    /**
     * Notify clients about a collision
     * @param client_fd Client that had collision
     * @param collision_type Type of collision ('c' for coin, 'e' for electric)
     * @param x X position of collision
     * @param y Y position of collision
     */
    void notifyCollision(int client_fd, char collision_type, int x, int y);

    /**
     * Start the game
     */
    void startGame();

    /**
     * End the game
     * @param winner_fd Socket of winning player, or -1 for no winner
     */
    void endGame(int winner_fd);
};

#endif // SERVER_HPP
