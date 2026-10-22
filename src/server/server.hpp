// The authoritative game server: it runs the game, the clients only send inputs and draw what it says.
// Split by theme: server.cpp (connections, messages), logic.cpp (lobby, physics, collisions),
// combat.cpp (CYBERPSYCHO attacks and drones), ultimates.cpp, titans.cpp, snipers.cpp, shift.cpp (the Attack Titan
// power-up) and powerups.cpp (shield, Sandevistan, Berserk).

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
#include "../common/snipers.hpp"
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
    static constexpr int CHUNKS_TO_MAX_DIFFICULTY = 30;   // ~1200 m

    // Run speed in tiles per second, ramps up over time: top speed after ~2 min, ~1500 m
    static constexpr float START_SPEED = 9.0f;
    static constexpr float MAX_SPEED = 16.0f;
    static constexpr float ACCELERATION = 0.06f;

    struct Titan {
        TitanType type;
        float x;        // left edge, in tiles
        float stagger = 0;  // seconds left standing still, shot by the Hunter: a cleaver cut kills it
    };

    struct Sniper {
        int target_fd;
        float y;        // its line, in tiles
        float time;     // seconds since it started aiming

        SniperPhase phase() const
        {
            if (time < SNIPER_TRACK_TIME)
                return SniperPhase::Tracking;
            return time < SNIPER_TRACK_TIME + SNIPER_AIM_TIME ? SniperPhase::Aiming : SniperPhase::Firing;
        }
    };

    struct Projectile {
        int owner_fd;
        float x;
        float y;
        float speed;
        float max_x;
        float radius;
        Weapon weapon;          // Mikasa's spears blow up where they stop, the Hunter's bullets stagger titans
    };

    // Johnny's fire wave, Jinx's Fishbones and the Hunter's Moonlight Blade, sweeping ahead and razing rows top to bottom
    struct Sweep {
        int owner_fd;
        float start;    // in tiles
        float front;
        float range;
        float speed;
        int top;
        int bottom;
        float delay = 0;    // seconds before it sets off: Fishbones charges first
    };

    // David's nuke, falling
    struct Nuke {
        int owner_fd;
        float impact;   // x where it lands, in tiles
        float timer;    // seconds before it does
    };

    ServerConfig config;
    int server_fd;
    std::atomic<bool> stopping;
    size_t required_players;

    // The run
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

    // CYBERPSYCHO
    std::set<std::pair<int, int>> destroyed_drones;
    std::vector<Projectile> projectiles;
    std::map<std::pair<int, int>, float> hacked_zappers;   // node -> seconds before it's live again (hacked or burnt)
    std::vector<Sweep> sweeps;
    std::vector<Nuke> nukes;

    std::vector<Titan> titans;
    float titan_timer;      // seconds before the next titan

    std::vector<Sniper> snipers;
    float sniper_timer;     // seconds before the next sniper aims

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

    void attractCoins(Player &player);
    void bounceHack(int client_fd, const Player &player, int from_x, int from_y);

    void sendGameState();

    void handlePlayerInput(int client_fd, bool jet_activated);

    void notifyCollision(char collision_type, const Player &player, int x, int y);

    void endGame(int winner_number);

    //===========================================================================
    // CYBERPSYCHO (combat.cpp)
    //===========================================================================

    void pressAttack(int client_fd);

    void releaseAttack(int client_fd);

    void handleAttack(int client_fd);

    void firePistol(int client_fd, Player &player);

    void announceShot(const Player &player, Weapon weapon);

    void fireProjectile(int client_fd, const Player &player, Weapon weapon);

    int strike(int client_fd, const Player &player, float reach);

    void cleave(int client_fd, Player &player, float reach);

    bool hack(int client_fd, Player &player);

    void updateProjectiles();

    bool projectileHit(const Projectile &shot, float from_x);

    float spearHit(const Projectile &shot, float from_x) const;

    bool bulletHit(const Projectile &shot, float from_x);

    void spearBlast(int owner_fd, float x);

    bool isLiveDrone(int x, int y) const;

    void destroyDrone(int owner_fd, int x, int y);

    //===========================================================================
    // Ultimates (ultimates.cpp)
    //===========================================================================

    void handleUltimate(int client_fd);

    void updateUltimates();

    void scorch(int owner_fd, float from, float to, float titans_from, int top = 0, int bottom = 1 << 20);

    void breach(int owner_fd, float start);

    //===========================================================================
    // Titans (titans.cpp)
    //===========================================================================

    void resetTitans();

    void spawnTitans(float leader_x);

    void updateTitans(float rear_x);

    bool hitByTitan(const Player &player) const;

    Titan *titanInPath(const Projectile &shot, float from_x);

    void slashTitans(int client_fd, const Player &player, float reach);

    void sendTitans();

    //===========================================================================
    // Arasaka snipers (snipers.cpp)
    //===========================================================================

    void resetSnipers();

    void spawnSnipers(float leader_x);

    void updateSnipers();

    bool hitBySniper(const Player &player) const;

    void parrySnipers(Player &player);

    void sendSnipers();

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
