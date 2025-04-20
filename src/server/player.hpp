/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef PLAYER_HPP
    #define PLAYER_HPP

class Player {
private:
    int client_fd;
    int player_number;

    // Position
    int x;
    int y;

    // Velocity
    float y_velocity;

    // Game state
    int score;
    bool jet_active;

    // Constants a modif si mal equilibré, juste faut mettre en commentaires les anciennes valeurs au cas ou
    static constexpr float GRAVITY = 0.5f;
    static constexpr float JET_POWER = -0.8f;
    static constexpr float MAX_VELOCITY = 2.0f;
    static constexpr float FORWARD_SPEED = 1.0f;

public:
    Player(int client_fd);

    int getClientFd() const { return client_fd; }
    int getPlayerNumber() const { return player_number; }
    void setPlayerNumber(int num) { player_number = num; }

    int getX() const { return x; }
    void setX(int val) { x = val; }

    int getY() const { return y; }
    void setY(int val) { y = val; }

    int getScore() const { return score; }
    void addScore(int points) { score += points; }

    bool isJetActive() const { return jet_active; }
    void setJetActive(bool active) { jet_active = active; }

    void moveForward();
    void applyPhysics();
};

#endif
