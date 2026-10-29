// Game state shared between the network thread and the render thread

#ifndef STATE_HPP
    #define STATE_HPP

#include <chrono>
#include <map>
#include <optional>
#include <mutex>
#include <set>
#include <utility>
#include <cstdint>
#include <vector>
#include "../common/messages.hpp"
#include "../common/powerups.hpp"

enum class Phase { Waiting, Countdown, Running, Over };

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
    std::set<std::pair<int, int>> pinata_drops;     // drones the Drone Pinata made drop a power-up
    std::vector<ShotEvent> pending_shots;
    std::vector<UltimateEvent> pending_ultimates;
    std::vector<TitanState> titans;
    std::vector<SniperState> snipers;
    BossState boss;
    std::set<std::pair<int, int>> my_coins;
    std::vector<CollisionEffect> pending_effects;
    std::vector<std::vector<uint8_t>> pending_map_data;
    bool rematch_requested = false;
    bool trauma_offer = false;      // the Trauma Team asks me whether to come, until I answer
    std::optional<std::chrono::steady_clock::time_point> attack_down;   // since when my attack key is down

public:
    void startCountdown(int my_number, const std::map<int, int> &roster, int district, GameMode mode);
    void pushShot(ShotEvent shot);
    void pushUltimate(UltimateEvent ultimate);
    void setTitans(std::vector<TitanState> list);
    void setSnipers(std::vector<SniperState> list);
    void setBoss(BossState state);
    void setCountdown(int value);
    void updatePlayer(int player_number, const PlayerState &player);
    void handleCollision(char type, int player, int x, int y);
    void setWinner(int player_number);
    void pushMapData(std::vector<uint8_t> data);
    bool requestRematch();

    void holdAttack(bool down);
    // Seconds my attack key has been down, 0 when it's up
    float attackHeld() const;
    Phase getPhase() const;
    int getCountdown() const;
    int getMyPlayerNumber() const;
    int getWinner() const;
    std::map<int, PlayerState> getPlayers() const;
    std::map<int, int> getCharacters() const;
    int getDistrict() const;
    GameMode getMode() const;
    std::set<std::pair<int, int>> getDestroyedDrones() const;
    std::set<std::pair<int, int>> getPinataDrops() const;
    std::vector<ShotEvent> takeShots();
    std::vector<UltimateEvent> takeUltimates();
    std::vector<TitanState> getTitans() const;
    std::vector<SniperState> getSnipers() const;
    BossState getBoss() const;
    std::set<std::pair<int, int>> getCollectedCoins() const;
    std::vector<CollisionEffect> takeEffects();
    std::vector<std::vector<uint8_t>> takeMapData();
    bool isRematchRequested() const;
    bool isTraumaOffered() const;
    // Answered: true if there was a question to answer
    bool answerTraumaOffer();
};

#endif
