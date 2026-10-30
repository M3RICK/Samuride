// Connection to the server: a network thread fills the GameState, the main thread renders it

#ifndef CLIENT_HPP
    #define CLIENT_HPP

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

#include "../common/protocol.hpp"
#include "../common/upgrades.hpp"

class GameState;

// What the player brings to the server. District and mode: -1 when joining, the host picks.
struct Loadout {
    int character = 0;
    int district = -1;      // -1 when joining: the host picks, like the mode
    int mode = -1;
    UpgradeLevels upgrades{};
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
    std::atomic<int> server_version = -1;   // set when the server refused ours (MSG_VERSION)

    std::vector<uint8_t> stream;    // bytes received, not yet a full packet

    std::queue<std::vector<uint8_t>> message_queue;
    std::mutex queue_mutex;

    bool connectToServer(int timeout_ms);
    void sendConnectMessage();

    void networkLoop();
    void processOutgoingMessages();
    void readIncomingData();
    void processMessage(const MessageHeader &header, const std::vector<uint8_t> &payload);
    void sendToServer(const std::vector<uint8_t> &data);

    void handleGameStart(const std::vector<uint8_t> &payload);
    void handleShot(const std::vector<uint8_t> &payload);
    void handleUltimate(const std::vector<uint8_t> &payload);
    void handleTitans(const std::vector<uint8_t> &payload);
    void handleSnipers(const std::vector<uint8_t> &payload);
    void handleBoss(const std::vector<uint8_t> &payload);
    void handleGameState(const std::vector<uint8_t> &payload);
    void handleCollision(const std::vector<uint8_t> &payload);
    void handleGameEnd(const std::vector<uint8_t> &payload);

public:
    // Outdated: the server speaks a newer protocol than this build; HostOutdated: an older one
    enum class RunResult { Menu, Quit, Outdated, HostOutdated };

    Client(const std::string &server_ip, int server_port, Loadout loadout, bool debug_mode, GameState &game_state);
    ~Client();

    bool initialize();
    RunResult run(InputManager &input_manager, Renderer &renderer);
    void stop();

    void sendPlayerInput(bool jet_activated);
    void sendAttack(bool pressed);
    void sendUltimate();
    // The Trauma Team's question: call it (true) or let me die
    void answerTraumaTeam(bool call);
    bool traumaOffered() const;
    // The Ripperdoc's implant, 1 to IMPLANTS, or 0 for none
    void answerRipperdoc(int implant);
    bool ripperdocOpen() const;
    // CYBERPSYCHO: the attack and ultimate buttons are on screen
    bool combatRun() const;
    // The run is over or the server is gone: the menu button is on screen
    bool runEnded() const;
};

#endif
