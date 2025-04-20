/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "state.hpp"

GameState::GameState() : winner(-1) {
}

void GameState::updatePlayer(int player_number, int x, int y, int score, bool jet_active)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    PlayerState& player = players[player_number];
    player.x = x;
    player.y = y;
    player.score = score;
    player.jet_active = jet_active;
}

void GameState::handleCollision(char type, int x, int y)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    effects.emplace_back(type, x, y);
}

void GameState::setWinner(int player_number)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    winner = player_number;
}

int GameState::getWinner() const
{
    return winner;
}

std::map<int, PlayerState> GameState::getPlayers()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return players;
}

std::vector<GameState::CollisionEffect> GameState::getEffects()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return effects;
}

void GameState::updateEffects()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    for (auto it = effects.begin(); it != effects.end(); ) {
        it->lifetime--;
        if (it->lifetime <= 0) {
            it = effects.erase(it);
        } else {
            it++;
        }
    }
}
