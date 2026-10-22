// Game state shared between the network thread and the render thread

#include "state.hpp"

void GameState::startCountdown(int my_number, const std::map<int, int> &roster, int run_district, GameMode run_mode)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    // Also the start of a rematch: wipe the previous run
    my_player_number = my_number;
    characters = roster;
    district = run_district;
    mode = run_mode;
    destroyed_drones.clear();
    pending_shots.clear();
    pending_ultimates.clear();
    titans.clear();
    snipers.clear();
    phase = Phase::Countdown;
    countdown = 0;
    winner = -1;
    rematch_requested = false;
    players.clear();
    my_coins.clear();
    pending_effects.clear();
}

void GameState::setCountdown(int value)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    countdown = value;
    if (value == 0)
        phase = Phase::Running;
}

void GameState::updatePlayer(int player_number, const PlayerState &player)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    players[player_number] = player;
}

void GameState::handleCollision(char type, int player, int x, int y)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    if ((type == COIN_TILE || type == SHIFT_TILE || isPowerUp(type)) && player == my_player_number)
        my_coins.insert({x, y});
    if (type == DRONE_TILE)
        destroyed_drones.insert({x, y});
    pending_effects.push_back({type, player, x, y});
}

void GameState::setWinner(int player_number)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    winner = player_number;
    phase = Phase::Over;
}

void GameState::pushMapData(std::vector<uint8_t> data)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    pending_map_data.push_back(std::move(data));
}

// Returns true the first time, so the request is only sent once
bool GameState::requestRematch()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    if (phase != Phase::Over || rematch_requested)
        return false;
    rematch_requested = true;
    return true;
}

Phase GameState::getPhase() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return phase;
}

int GameState::getCountdown() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return countdown;
}

int GameState::getMyPlayerNumber() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return my_player_number;
}

int GameState::getWinner() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return winner;
}

std::map<int, PlayerState> GameState::getPlayers() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return players;
}

void GameState::pushShot(ShotEvent shot)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    pending_shots.push_back(shot);
}

void GameState::pushUltimate(UltimateEvent ultimate)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    pending_ultimates.push_back(ultimate);
}

void GameState::setTitans(std::vector<TitanState> list)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    titans = std::move(list);
}

std::vector<TitanState> GameState::getTitans() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return titans;
}

void GameState::setSnipers(std::vector<SniperState> list)
{
    std::lock_guard<std::mutex> lock(state_mutex);

    snipers = std::move(list);
}

std::vector<SniperState> GameState::getSnipers() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return snipers;
}

GameMode GameState::getMode() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return mode;
}

std::set<std::pair<int, int>> GameState::getDestroyedDrones() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return destroyed_drones;
}

std::vector<ShotEvent> GameState::takeShots()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return std::exchange(pending_shots, {});
}

std::vector<UltimateEvent> GameState::takeUltimates()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return std::exchange(pending_ultimates, {});
}

int GameState::getDistrict() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return district;
}

std::map<int, int> GameState::getCharacters() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return characters;
}

std::set<std::pair<int, int>> GameState::getCollectedCoins() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return my_coins;
}

std::vector<CollisionEffect> GameState::takeEffects()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return std::exchange(pending_effects, {});
}

std::vector<std::vector<uint8_t>> GameState::takeMapData()
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return std::exchange(pending_map_data, {});
}

bool GameState::isRematchRequested() const
{
    std::lock_guard<std::mutex> lock(state_mutex);

    return rematch_requested;
}
