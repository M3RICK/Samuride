// Game state shared between the network thread and the render thread

#ifndef STATE_HPP
    #define STATE_HPP

#include <map>
#include <mutex>
#include <set>
#include <utility>
#include <cstdint>
#include <vector>
#include "../common/cyberpsycho.hpp"
#include "../common/titans.hpp"
#include "../common/snipers.hpp"
#include "../common/shift.hpp"
#include "../common/powerups.hpp"

enum class Phase { Waiting, Countdown, Running, Over };

struct PlayerState {
    float x = 0;
    float y = 0;
    int score = 0;
    bool jet_active = false;
    Form form = Form::Normal;
    bool shielded = false;
    bool sprinting = false;     // Sandevistan
    int ram = 0;                // Lucy's hack charges
    bool berserk = false;
    int ultimate = 0;           // gauge, in percent
    int spears = 0;             // Mikasa's thunder spears left
    bool grace = false;         // invincible for a second after a shield, a sprint or a titan
    int style = 0;              // Dante's style rank, 0 (D) to 6 (SSS)
};

// Someone attacked in CYBERPSYCHO
struct ShotEvent {
    int player;
    Weapon weapon;
};

// Someone fired their ultimate
struct UltimateEvent {
    int player;
    Ultimate ultimate;
    float x;    // where it starts, in tiles
    float y;    // the line it follows, for Fishbones
};

struct TitanState {
    TitanType type;
    float x;    // left edge, in tiles
};

struct SniperState {
    SniperPhase phase;
    float y;        // its line, in tiles
};

struct CollisionEffect {
    char type;
    int player;
    int x;
    int y;
};

// Written by the network thread, read by the render thread
class GameState {
private:
    mutable std::mutex state_mutex;

    Phase phase = Phase::Waiting;
    int countdown = 0;
    int my_player_number = -1;
    int winner = -1;

    std::map<int, PlayerState> players;
    std::map<int, int> characters;
    int district = 0;
    GameMode mode = GameMode::Classic;
    std::set<std::pair<int, int>> destroyed_drones;
    std::vector<ShotEvent> pending_shots;
    std::vector<UltimateEvent> pending_ultimates;
    std::vector<TitanState> titans;
    std::vector<SniperState> snipers;
    std::set<std::pair<int, int>> my_coins;
    std::vector<CollisionEffect> pending_effects;
    std::vector<std::vector<uint8_t>> pending_map_data;
    bool rematch_requested = false;

public:
    void startCountdown(int my_number, const std::map<int, int> &roster, int district, GameMode mode);
    void pushShot(ShotEvent shot);
    void pushUltimate(UltimateEvent ultimate);
    void setTitans(std::vector<TitanState> list);
    void setSnipers(std::vector<SniperState> list);
    void setCountdown(int value);
    void updatePlayer(int player_number, const PlayerState &player);
    void handleCollision(char type, int player, int x, int y);
    void setWinner(int player_number);
    void pushMapData(std::vector<uint8_t> data);
    bool requestRematch();

    Phase getPhase() const;
    int getCountdown() const;
    int getMyPlayerNumber() const;
    int getWinner() const;
    std::map<int, PlayerState> getPlayers() const;
    std::map<int, int> getCharacters() const;
    int getDistrict() const;
    GameMode getMode() const;
    std::set<std::pair<int, int>> getDestroyedDrones() const;
    std::vector<ShotEvent> takeShots();
    std::vector<UltimateEvent> takeUltimates();
    std::vector<TitanState> getTitans() const;
    std::vector<SniperState> getSnipers() const;
    std::set<std::pair<int, int>> getCollectedCoins() const;
    std::vector<CollisionEffect> takeEffects();
    std::vector<std::vector<uint8_t>> takeMapData();
    bool isRematchRequested() const;
};

#endif
