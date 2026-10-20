// The authoritative game server, split by theme over server, logic, combat, titans and shift .cpp

#ifndef SERVER_HPP
    #define SERVER_HPP

#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include "../common/socket.hpp"
#include "../common/map.hpp"
#include "../common/protocol.hpp"
#include "../common/cyberpsycho.hpp"
#include "../common/titans.hpp"
#include "player.hpp"

struct ServerConfig {
    int port = -1;
    std::string map_path;       // optional hand-made start of the run
    bool solo = false;          // one player, on loopback only
    bool debug_mode = false;
    bool quiet = false;         // no status lines: when running inside the game
};

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
    static constexpr float START_SPEED = 9.0f;
    static constexpr float MAX_SPEED = 18.0f;
    static constexpr float ACCELERATION = 0.15f;

    struct Titan {
        TitanType type;
        float x;        // left edge, in tiles
    };

    struct Projectile {
        int owner_fd;
        float x;
        float y;
        float speed;
        float max_x;
        float radius;
    };

    ServerConfig config;
    int server_fd;
    std::atomic<bool> stopping;
    size_t required_players;

    Map game_map;
    std::mt19937 rng;
    int chunks_generated;
    int last_orb_chunk;     // chunk of the last Attack Titan orb
    float run_speed;

    Phase phase;
    int countdown;
    int countdown_ticks;
    std::set<int> ready_clients;

    // Picked by the first player in the lobby: the host
    uint8_t district;
    GameMode mode;

    std::set<std::pair<int, int>> destroyed_drones;
    std::vector<Projectile> projectiles;
    // Johnny's ultimate, sweeping ahead
    struct FireWave {
        int owner_fd;
        float start;    // in tiles
        float front;
    };

    std::map<std::pair<int, int>, float> hacked_zappers;   // node -> seconds before it's live again (hacked or burnt)
    std::vector<FireWave> fire_waves;

    // David's ultimate, falling
    struct Nuke {
        int owner_fd;
        float impact;   // x where it lands, in tiles
        float timer;    // seconds before it does
    };

    std::vector<Nuke> nukes;

    std::vector<Titan> titans;
    float titan_timer;      // seconds before the next titan

    std::vector<Socket::PollEntry> poll_fds;
    std::map<int, Player> players;

    // More unparsed bytes than this from a client is garbage
    static constexpr size_t MAX_PENDING_BYTES = 1024;
    // A client this far behind on what we send (minutes of game state) isn't reading anymore
    static constexpr size_t MAX_SEND_BACKLOG = 1 << 20;

    //===========================================================================
    // Server Initialization
    //===========================================================================


    bool resetMap();

    void extendMap(float leader_x);

    //===========================================================================
    // Client Connection Management
    //===========================================================================

    void acceptNewClient();

    void removeClient(int client_fd);

    void handlePlayerDisconnection();

    //===========================================================================
    // Client Data Handling
    //===========================================================================

    void handleClientData(int client_fd);

    void processClientMessage(int client_fd, const MessageHeader &header, const std::vector<uint8_t> &payload);

    void handleConnect(int client_fd, const std::vector<uint8_t> &payload);

    //===========================================================================
    // Socket Event Processing
    //===========================================================================

    void processSocketEvents();

    void updateGameState();

    void announce(const std::string &message) const;

    //===========================================================================
    // Network Communication
    //===========================================================================

    void sendToClient(int client_fd, const std::vector<uint8_t> &data);

    void flush(Player &player);

    void flushClients();

    void broadcastToAllClients(const std::vector<uint8_t> &data);

    //===========================================================================
    // Game Logic
    //===========================================================================

    bool lobbyReady() const;
    void startGame();

    void reopenLobby();

    void handleReady(int client_fd);

    void updateCountdown();

    void updatePlayers();

    bool checkPlayerCollisions(Player &player);

    void sendGameState();

    void handlePlayerInput(int client_fd, bool jet_activated);

    void notifyCollision(char collision_type, const Player &player, int x, int y);

    void endGame(int winner_number);

    //===========================================================================
    // CYBERPSYCHO (combat.cpp)
    //===========================================================================

    void handleAttack(int client_fd);

    void strike(int client_fd, const Player &player, float reach);

    bool hack(int client_fd, Player &player);

    void updateProjectiles();

    bool projectileHit(const Projectile &shot, float from_x);

    bool isLiveDrone(int x, int y) const;

    void destroyDrone(int owner_fd, int x, int y);

    void handleUltimate(int client_fd);

    void updateUltimates();

    void scorch(int owner_fd, float from, float to, float titans_from);

    void breach(int owner_fd, float start);

    //===========================================================================
    // Titans (titans.cpp)
    //===========================================================================

    void resetTitans();

    void spawnTitans(float leader_x);

    void updateTitans(float rear_x);

    bool hitByTitan(const Player &player) const;

    void slashTitans(int client_fd, const Player &player, float reach);

    void sendTitans();

    //===========================================================================
    // Attack Titan power-up (shift.cpp)
    //===========================================================================

    void placeShiftOrb(std::vector<std::string> &rows);

    void titanSmash(Player &player);

    //===========================================================================
    // Shield & Sandevistan (powerups.cpp)
    //===========================================================================

    bool placeInFreeTile(std::vector<std::string> &rows, char tile);

    void placePowerUp(std::vector<std::string> &rows);

    char powerUpAt(int x, int y) const;

    bool hurt(Player &player, char cause, int x, int y);

public:
    explicit Server(const ServerConfig &config);

    ~Server();

    bool initialize();

    // Runs until stop(), from any thread
    void run();
    void stop() { stopping = true; }
};

#endif
