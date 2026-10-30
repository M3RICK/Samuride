// Game rules: lobby, countdown, endless map, physics ticks, collisions, game over

#include "server.hpp"
#include "generator.hpp"
#include "../common/debug.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

// A fresh run: the map file if there is one (the run carries on procedurally past its end),
// otherwise a short empty runway
bool Server::resetMap()
{
    game_map.clear();
    chunks_generated = 0;
    last_orb_chunk = -SHIFT_MIN_GAP_CHUNKS;
    destroyed_drones.clear();
    pinata_drops.clear();
    trauma_offers.clear();
    ripperdoc_waiting.clear();
    projectiles.clear();
    hacked_zappers.clear();
    titans.reset();
    snipers.reset();
    boss.reset();

    if (!config.map_path.empty()) {
        if (!game_map.loadFromFile(config.map_path)) {
            std::cerr << "Invalid map file: " << config.map_path << std::endl;
            return false;
        }
    } else {
        game_map.appendChunk(std::vector<std::string>(MAP_HEIGHT, std::string(SAFE_START, EMPTY_TILE)));
    }
    extendMap(0);
    return true;
}

void Server::extendMap(float leader_x)
{
    while (game_map.getWidth() < leader_x + LOOKAHEAD) {
        size_t from = game_map.getWidth();
        // Adam Smasher's arena: nothing in the way but him, and its empty chunks don't make what follows harder
        bool arena = boss.holds(from) || boss.holds(from + CHUNK_WIDTH - 1);
        float difficulty = std::min(1.0f, chunks_generated / static_cast<float>(CHUNKS_TO_MAX_DIFFICULTY));
        chunks_generated += !arena;

        bool drones = mode == GameMode::Cyberpsycho;
        std::vector<std::string> rows = arena ? std::vector<std::string>(game_map.getHeight(), std::string(CHUNK_WIDTH, EMPTY_TILE))
                                              : generateChunk(rng, CHUNK_WIDTH, game_map.getHeight(), difficulty, drones,
                                                              WANTED_DRONES_PER_STAR * wantedStars());
        if (!arena) {
            placeShiftOrb(rows);
            placePowerUp(rows);
        }
        game_map.appendChunk(rows);

        // Before the run, clients get the whole map in one go instead
        if (phase == Phase::Running)
            broadcastToAllClients(Protocol::createPacket(MSG_MAP_DATA, game_map.serialize(from)));
    }
}

void Server::updateGameState()
{
    switch (phase) {
        case Phase::Waiting:
            if (lobbyReady())
                startGame();
            break;
        case Phase::Countdown:
            updateCountdown();
            break;
        case Phase::Running:
            updatePlayers();
            break;
        case Phase::Over:
            break;
    }
}

// Full, and everyone said who they play: a game started on accept would miss the loadouts
bool Server::lobbyReady() const
{
    return players.size() == required_players &&
           std::all_of(players.begin(), players.end(), [](const auto &entry) { return entry.second.isIntroduced(); });
}

// Everyone gets the run's map, then who plays whom
void Server::startGame()
{
    int player_num = 0;
    GameStart start;
    start.district = district;
    start.mode = mode;

    broadcastToAllClients(Protocol::createPacket(MSG_MAP_DATA, game_map.serialize()));

    for (auto &[client_fd, player] : players) {
        player.reset(player_num, game_map.getHeight() - Player::HEIGHT);
        start.roster[player_num++] = player.getCharacter();
    }

    // Each client gets its own number first, then who plays which character, then the district
    for (auto &[client_fd, player] : players) {
        start.my_number = player.getPlayerNumber();
        sendToClient(client_fd, Protocol::createPacket(MSG_GAME_START, Message::encode(start)));
    }

    phase = Phase::Countdown;
    countdown = COUNTDOWN_SECONDS;
    countdown_ticks = 0;
    run_speed = START_SPEED;
    slowed_after_boss = false;
    ready_clients.clear();

    announce("Game starting with " + std::to_string(players.size()) + " players");
}

// Rematch once everyone in the lobby asked for one
void Server::handleReady(int client_fd)
{
    if (phase != Phase::Over)
        return;

    ready_clients.insert(client_fd);
    if (players.size() < required_players || ready_clients.size() < players.size())
        return;

    resetMap();
    startGame();
}

void Server::updateCountdown()
{
    if (countdown_ticks > 0) {
        countdown_ticks--;
        return;
    }

    broadcastToAllClients(Protocol::createPacket(MSG_COUNTDOWN, { static_cast<uint8_t>(countdown) }));
    sendGameState();

    if (countdown == 0) {
        phase = Phase::Running;
        return;
    }
    countdown--;
    countdown_ticks = TICK_RATE - 1;
}

// The run never ends on its own: it stops at the first death
void Server::updatePlayers()
{
    float max_y = game_map.getHeight() - Player::HEIGHT;
    float leader_x = 0;
    float rear_x = 1e9f;
    std::vector<int> survivors;

    // A deadly hit asking whether to call the Trauma Team: the run waits for the answer, which is yes
    // once it is late
    if (!trauma_offers.empty()) {
        std::vector<int> late;
        for (auto &[client_fd, offer] : trauma_offers) {
            if ((offer.left -= TICK_DT) <= 0)
                late.push_back(client_fd);
        }
        for (int client_fd : late)
            answerTraumaTeam(client_fd, true);
        sendGameState();
        return;
    }
    // At the Ripperdoc's: the run waits for everyone's pick, or for the time to run out
    if (!ripperdoc_waiting.empty()) {
        if ((ripperdoc_left -= TICK_DT) <= 0)
            ripperdoc_waiting.clear();
        sendGameState();
        return;
    }

    // The Trauma Team at work on someone: the whole run waits for it
    bool waiting = false;
    for (auto &[client_fd, player] : players)
        waiting |= player.awaitRescue(TICK_DT, max_y);
    if (waiting) {
        sendGameState();
        return;
    }

    // Smasher's arena: the pace holds while he fights, then drops once he's behind
    float front = 0;
    for (const auto &[client_fd, player] : players)
        front = std::max(front, player.getX());
    if (!boss.holds(front))
        run_speed = std::min(MAX_SPEED, run_speed + ACCELERATION * TICK_DT);
    if (!slowed_after_boss && front >= BOSS_DISTANCE + BOSS_ARENA) {
        run_speed = std::max(START_SPEED, run_speed - BOSS_SPEED_DROP);
        slowed_after_boss = true;
        // ... and the Ripperdoc opens (answerRipperdoc)
        if (mode == GameMode::Cyberpsycho) {
            ripperdoc_left = RIPPERDOC_TIME;
            for (auto &[client_fd, player] : players) {
                ripperdoc_waiting.insert(client_fd);
                notifyCollision(RIPPERDOC_OFFER, player, static_cast<int>(player.getX()), static_cast<int>(player.getY()));
            }
        }
    }

    for (auto &[client_fd, player] : players) {
        player.update(TICK_DT, run_speed, max_y);
        // Berserk attacks on its own, as soon as the cooldown allows
        if (player.isBerserk() && player.weapon().autoFires())
            handleAttack(client_fd);
        // What the weapon does on its own: the rest of a Pow-Pow burst
        if (mode == GameMode::Cyberpsycho)
            player.weapon().act(*this, player);
        leader_x = std::max(leader_x, player.getX());
        rear_x = std::min(rear_x, player.getX());
    }

    // Wraith's Stillness: the titans, the snipers and Smasher stand still, even the sights stop following
    bool frozen = std::ranges::any_of(players, [](const auto &entry) { return entry.second.ultimate().freezes(); });
    titans.update(rear_x, frozen ? 0 : TICK_DT);
    if (!frozen) {
        snipers.update(TICK_DT, players);
        boss.update(leader_x, TICK_DT, rng, players, game_map.getHeight());
    }
    // Hacked zappers come back on
    for (auto &[node, left] : hacked_zappers)
        left -= TICK_DT;
    std::erase_if(hacked_zappers, [](const auto &entry) { return entry.second <= 0; });
    for (auto &[client_fd, player] : players) {
        if (!checkPlayerCollisions(player))
            survivors.push_back(player.getPlayerNumber());
    }

    // No titan nor sniper in Smasher's arena, nor charging into it
    if (!frozen && !boss.holds(leader_x) && !boss.holds(leader_x + LOOKAHEAD)) {
        titans.spawn(leader_x, TICK_DT, rng);
        // The NCPD sends them faster as the wanted level climbs
        snipers.spawn(leader_x, TICK_DT * (1 + WANTED_SNIPERS_PER_STAR * wantedStars()), rng, players);
    }
    updateProjectiles();
    updateUltimates();
    extendMap(leader_x);
    sendGameState();
    broadcastToAllClients(Protocol::createPacket(MSG_TITANS, Message::encode(titans.states())));
    broadcastToAllClients(Protocol::createPacket(MSG_SNIPERS, Message::encode(snipers.states())));
    broadcastToAllClients(Protocol::createPacket(MSG_BOSS, Message::encode(boss.state())));

    if (survivors.size() < players.size())
        endGame(survivors.size() == 1 && !config.solo ? survivors[0] : -1);
}

// Samples the body from head to feet. Returns true when the player got zapped or run down.
bool Server::checkPlayerCollisions(Player &player)
{
    float px = player.getX() + 0.5f;

    if (player.getForm() == Form::Titan) {
        titanSmash(player);
        return false;
    }
    if (player.getForm() == Form::Shifting)
        return false;
    attractCoins(player);

    for (float dy : {0.3f, 1.0f, 1.7f}) {
        float py = player.getY() + dy;
        int tile_x = static_cast<int>(px);
        int tile_y = static_cast<int>(py);
        char tile = game_map.getTile(tile_x, tile_y);

        if (pickUp(player, tile_x, tile_y)) {
        } else if (tile == SHIFT_TILE && player.getForm() == Form::Normal && player.takePickup(tile_x, tile_y)) {
            player.startShift();
            notifyCollision(SHIFT_TILE, player, tile_x, tile_y);
            return false;
        } else if (player.isInvincible()) {
            continue;
        } else if (((tile == ZAPPER_TILE && !hacked_zappers.count({tile_x, tile_y})) || isLiveDrone(tile_x, tile_y)) &&
                   std::abs(px - tile_x - 0.5f) < 0.4f && std::abs(py - tile_y - 0.5f) < 0.4f) {
            // Zappers and drones only bite near their core, grazing the tile edge is fine
            return hurt(player, ZAPPER_TILE, tile_x, tile_y);
        }
    }

    if (!player.isInvincible() && titans.hit(player, game_map.getHeight()))
        return hurt(player, TITAN_HIT, player.getX(), player.getY() + 1);
    if (!player.isInvincible() && snipers.hit(player))
        return hurt(player, SNIPER_HIT, player.getX(), player.getY() + 1);
    if (!player.isInvincible() && boss.hit(player, game_map.getHeight()))
        return hurt(player, BOSS_HIT, player.getX(), player.getY() + 1);
    return false;
}

bool Server::pickUp(Player &player, int x, int y)
{
    char power = powerUpAt(x, y);

    if (game_map.getTile(x, y) == COIN_TILE) {
        collectEddy(player, x, y);
        return true;
    }
    if (power == EMPTY_TILE || !player.takePickup(x, y))
        return false;
    player.powerUp(power);
    notifyCollision(power, player, x, y);
    return true;
}

// The eddie magnet upgrade: eddies whose center is within its reach of the body are picked up too.
// A round reach, so it grabs what you graze, not the whole column.
void Server::attractCoins(Player &player)
{
    float reach = magnetReach(player.getUpgrades());
    float left = player.getX();
    float top = player.getY();

    for (int x = std::max(0, static_cast<int>(left - reach)); reach > 0 && x <= left + 1 + reach; x++) {
        for (int y = std::max(0, static_cast<int>(top - reach)); y <= top + Player::HEIGHT + reach; y++) {
            float dx = std::max({left - x - 0.5f, x + 0.5f - left - 1, 0.0f});
            float dy = std::max({top - y - 0.5f, y + 0.5f - top - Player::HEIGHT, 0.0f});
            if (dx * dx + dy * dy <= reach * reach && game_map.getTile(x, y) == COIN_TILE)
                collectEddy(player, x, y);
        }
    }
}

// Each eddy once per player: double for the titan on the Titan Serum, and the Lucky Chip's jackpot
void Server::collectEddy(Player &player, int x, int y)
{
    if (!player.collectCoin(x, y))
        return;
    notifyCollision(COIN_TILE, player, x, y);
    if (player.getForm() == Form::Titan && player.getUpgrades()[UPGRADE_SERUM] > 0)
        player.addScore(1);

    float jackpot = jackpotChance(player.getUpgrades());
    if (jackpot > 0 && std::uniform_real_distribution<float>(0, 1)(rng) < jackpot) {
        player.addScore(JACKPOT_EDDIES - 1);
        notifyCollision(JACKPOT, player, x, y);
    }
}

void Server::sendGameState()
{
    std::map<int, PlayerState> states;

    for (const auto &[client_fd, player] : players) {
        states[player.getPlayerNumber()] = {
            .x = player.getX(),
            .y = player.getY(),
            .score = player.getScore(),
            .jet_active = player.isJetActive(),
            .form = player.getForm(),
            .shielded = player.hasShield(),
            .sprinting = player.isSprinting(),
            .ram = player.weapon().charges(),
            .berserk = player.isBerserk(),
            .ultimate = player.ultimatePercent(),
            .spears = player.ultimate().charges(),  // Mikasa's thunder spears left
            .grace = player.isGracing(),            // shield broken, Sandevistan or titan over
            .style = player.weapon().style(),
            .wanted = player.wantedStars(),
        };
    }

    broadcastToAllClients(Protocol::createPacket(MSG_GAME_STATE, Message::encode(states)));
}

void Server::handlePlayerInput(int client_fd, bool jet_activated)
{
    auto it = players.find(client_fd);

    if (it == players.end())
        return;

    it->second.setJetActive(jet_activated);
    DEBUG_LOG("Player " + std::to_string(it->second.getPlayerNumber()) +
              " jet " + (jet_activated ? "ON" : "OFF"));
}

void Server::notifyCollision(char collision_type, const Player &player, int x, int y)
{
    CollisionEffect collision{collision_type, player.getPlayerNumber(), x, y};

    broadcastToAllClients(Protocol::createPacket(MSG_COLLISION, Message::encode(collision)));
}

void Server::endGame(int winner_number)
{
    if (phase != Phase::Countdown && phase != Phase::Running)
        return;

    phase = Phase::Over;

    uint8_t winner = winner_number >= 0 ? winner_number : 0xFF;
    broadcastToAllClients(Protocol::createPacket(MSG_GAME_END, { winner }));

    if (winner_number >= 0)
        announce("Game over, player " + std::to_string(winner_number) + " wins");
    else
        announce("Game over, no winner");
}
