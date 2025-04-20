/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

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

    PlayerState() : x(0), y(0), score(0), jet_active(false){}
};

class GameState {
public: //DUCP on a pas d animations ca sert a r
    struct CollisionEffect {
        char type;
        int x;
        int y;
        int lifetime;

        CollisionEffect(char type, int x, int y)
            : type(type), x(x), y(y), lifetime(20) {}
    };

private:
    std::map<int, PlayerState> players;
    std::mutex state_mutex;
    int winner;

    std::vector<CollisionEffect> effects;

public:
    GameState();

    void updatePlayer(int player_number, int x, int y, int score, bool jet_active);
    void handleCollision(char type, int x, int y);
    void setWinner(int player_number);
    int getWinner() const;

    // mutex
    std::map<int, PlayerState> getPlayers();
    std::vector<CollisionEffect> getEffects();

    void updateEffects();
};
#endif
