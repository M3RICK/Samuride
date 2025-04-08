#ifndef STATE_HPP
#define STATE_HPP

#include <map>
#include <mutex>
#include <vector>
#include "../common/map.hpp"

struct PlayerState {
    int x;
    int y;
    int score;
    bool jet_active;

    PlayerState() : x(0), y(0), score(0), jet_active(false) {}
};

class GameState {
private:
    std::map<int, PlayerState> players;
    std::mutex state_mutex;
    int winner;

    // Coin and collision effects
    struct CollisionEffect {
        char type;
        int x;
        int y;
        int lifetime;

        CollisionEffect(char type, int x, int y)
            : type(type), x(x), y(y), lifetime(20) {}
    };

    std::vector<CollisionEffect> effects;

public:
    GameState();

    void updatePlayer(int player_number, int x, int y, int score, bool jet_active);
    void handleCollision(char type, int x, int y);
    void setWinner(int player_number);
    int getWinner() const;

    // Data access (thread-safe)
    std::map<int, PlayerState> getPlayers();
    std::vector<CollisionEffect> getEffects();
    void updateEffects();
};

#endif // STATE_HPP
