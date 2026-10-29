// The authoritative game server: it runs the game, the clients only send inputs and draw what it says.
// Split by theme: server.cpp (connections, messages), logic.cpp (lobby, physics, collisions),
// combat.cpp (CYBERPSYCHO attacks, ultimates and drones; what each runner's do is in arsenal.cpp),
// titans.cpp, snipers.cpp, shift.cpp (the Attack Titan power-up) and powerups.cpp (shield, Sandevistan,
// Berserk).

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
#include "titans.hpp"
#include "snipers.hpp"
#include "boss.hpp"
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

    static constexpr int COUNTDOWN_SECONDS = 3;

    // Endless run: the map is generated CHUNK_WIDTH columns at a time, LOOKAHEAD columns ahead of the leader
    static constexpr size_t MAP_HEIGHT = 10;
    static constexpr size_t CHUNK_WIDTH = 40;
    static constexpr size_t SAFE_START = 14;
    static constexpr float LOOKAHEAD = 40;
    // The run's speed and difficulty: common/difficulty.hpp

    struct Projectile {
        int owner_fd;
        float x;
        float y;
        float speed;
        float max_x;
        float radius;
        Weapon weapon;          // Mikasa's spears blow up where they stop
    };

    ServerConfig config;
    int server_fd = -1;
    std::atomic<bool> stopping = false;
    size_t required_players;

    // The run
    Map game_map;
    std::mt19937 rng{std::random_device{}()};
    int chunks_generated = 0;
    int last_orb_chunk = -SHIFT_MIN_GAP_CHUNKS;     // chunk of the last Attack Titan orb
    float run_speed = START_SPEED;
    Phase phase = Phase::Waiting;
    int countdown = 0;
    int countdown_ticks = 0;
    std::set<int> ready_clients;

    // Picked by the host: the first player in the lobby, or whoever stayed when they left
    int host_fd = -1;
    uint8_t district = 0;
    GameMode mode = GameMode::Classic;

    // CYBERPSYCHO
    std::set<std::pair<int, int>> destroyed_drones;
    std::set<std::pair<int, int>> pinata_drops;         // drones the Drone Pinata made drop a power-up
    std::vector<Projectile> projectiles;
    std::map<std::pair<int, int>, float> hacked_zappers;   // node -> seconds before it's live again (hacked or burnt)

    Horde titans;
    SniperSquad snipers;
    Boss boss;
    // Deadly hits waiting on their player's answer, whether to call the Trauma Team: client fd -> the hit
    struct TraumaOffer {
        char cause;
        int x;
        int y;
        float left;     // seconds before it comes anyway
    };
    std::map<int, TraumaOffer> trauma_offers;

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

    // False when the client has to go
    bool processClientMessage(int client_fd, const MessageHeader &header, const std::vector<uint8_t> &payload);

    bool handleConnect(int client_fd, const std::vector<uint8_t> &payload);

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

    void collectEddy(Player &player, int x, int y);

    void sendGameState();

    void handlePlayerInput(int client_fd, bool jet_activated);

    void endGame(int winner_number);

    //===========================================================================
    // CYBERPSYCHO (combat.cpp); the runners' weapons and ultimates are in arsenal.cpp
    //===========================================================================

    void handleAttack(int client_fd);
    void releaseAttack(int client_fd);

    void handleUltimate(int client_fd);

    void updateUltimates();

    void updateProjectiles();

    bool projectileHit(const Projectile &shot, float from_x);

    float spearHit(const Projectile &shot, float from_x);

    void spearBlast(int owner_fd, float x);

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
    // Yes: the rescue; no: the death the hit was
    void answerTraumaTeam(int client_fd, bool call);

public:
    static constexpr int TICK_RATE = 60;
    static constexpr float TICK_DT = 1.0f / TICK_RATE;

    explicit Server(const ServerConfig &config);

    ~Server();

    bool initialize();

    // Runs until stop(), from any thread
    void run();
    void stop() { stopping = true; }

    //===========================================================================
    // What the runners' weapons and ultimates act on (arsenal.cpp)
    //===========================================================================

    const Map &map() const { return game_map; }

    // Every client plays the attacker's attack animation and sound (MSG_SHOT)
    void announceShot(const Player &player, Weapon weapon);

    void fireProjectile(const Player &player, Weapon weapon);
    // ... from a body whose top is at y, with no announce: the clients draw it themselves (Dante's double)
    void fireProjectile(const Player &player, Weapon weapon, float y);

    // Every live drone within reach in front of the player goes down. Returns how many.
    int strike(const Player &player, float reach) { return strike(player, reach, player.getY()); }
    // ... in front of a body whose top is at y
    int strike(const Player &player, float reach, float y);

    // The live drones a strike would take down, or a titan in its reach: Dante cuts only then
    bool inReach(const Player &player, float reach, float y) const;

    // Every titan within reach in front of the player goes down
    void slashTitans(Player &player, float reach);

    // CYBERPSYCHO: a hit on Adam Smasher while he fights, his bounty if it downs him. False when he isn't there.
    bool hitBoss(Player &player, int damage);
    // Wraith's Blink: the rockets it sweeps through turn back (how many), and the seconds a strike of
    // Smasher's about to land on him lasts (0 if none: Boss::threatens)
    int reflectRockets(float left, float right, float top, float bottom)
    {
        return boss.reflect(left, right, top, bottom, game_map.getHeight());
    }
    float bossThreatens(const Player &player) const
    {
        return boss.threatens(player, game_map.getHeight(), BLINK_PERFECT_WINDOW);
    }
    // Smasher fighting with his body (half a tile before his x to 2.5 after) between these two
    bool bossBetween(float from, float to) const
    {
        return boss.fighting() && boss.x() - 0.5f < to && boss.x() + 2.5f > from;
    }
    // ... with a blade, if he is within its reach (close by after a slam)
    bool slashBoss(Player &player, float reach);

    // Its bounty to whoever took it down, standing at x
    void titanDown(Player &player, float x);

    // Every column from `from` to `to` burns, rows top to bottom (floor to ceiling by default)
    void scorch(int owner_fd, float from, float to, float titans_from, int top = 0, int bottom = 1 << 20);

    bool isLiveDrone(int x, int y) const;

    void destroyDrone(int owner_fd, int x, int y);
    // The eddie or the power-up on this tile, if any is left: true when he took one
    bool pickUp(Player &player, int x, int y);

    // A drone shot down, or a zapper switched off
    bool isOff(int x, int y) const { return destroyed_drones.count({x, y}) || hacked_zappers.count({x, y}); }

    // Off for at least that many seconds
    void switchOffZapper(int x, int y, float seconds);

    void notifyCollision(char collision_type, const Player &player, int x, int y);
};

#endif
