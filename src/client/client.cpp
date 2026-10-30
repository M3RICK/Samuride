// Connection to the server: a network thread fills the GameState, the main thread renders it

#include "client.hpp"
#include "state.hpp"
#include "render/renderer.hpp"
#include "inputs.hpp"
#include "../common/debug.hpp"
#include "../common/socket.hpp"

#include <iostream>
#include <map>
#include <cstring>

Client::Client(const std::string& server_ip, int server_port, Loadout loadout, bool debug_mode, GameState &game_state)
    : client_fd(-1), server_ip(server_ip), server_port(server_port), loadout(loadout),
      game_state(game_state), connected(false), running(false) {
    g_logger.setDebugMode(debug_mode);
}

Client::~Client()
{
    stop();

    if (client_fd >= 0)
        Socket::close(client_fd);
}

// Keeps trying for a few seconds: a server we just launched may not be listening yet.
// An address nobody answers at gives up then, instead of freezing the game.
bool Client::initialize()
{
    using clock = std::chrono::steady_clock;
    auto deadline = clock::now() + std::chrono::seconds(3);

    if (!Socket::startup())
        return false;
    while (true) {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now()).count();
        if (left <= 0)
            return false;
        if (connectToServer(left))
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool Client::connectToServer(int timeout_ms)
{
    client_fd = Socket::connect(server_ip, server_port, timeout_ms);
    if (client_fd < 0)
        return false;

    DEBUG_LOG("Connected to server: " + server_ip + ":" + std::to_string(server_port));
    connected = true;
    sendToServer(Protocol::createPacket(MSG_VERSION, {PROTOCOL_VERSION}));
    sendConnectMessage();
    return true;
}

// When joining, the host's district and mode apply
void Client::sendConnectMessage()
{
    bool joining = loadout.district < 0 || loadout.mode < 0;
    ConnectRequest connect{.character = loadout.character, .upgrades = loadout.upgrades};

    if (!joining) {
        connect.district = loadout.district;
        connect.mode = loadout.mode;
    }
    sendToServer(Protocol::createPacket(MSG_CONNECT, Message::encode(connect)));
}

Client::RunResult Client::run(InputManager &input, Renderer &renderer)
{
    using clock = std::chrono::steady_clock;
    Phase last_phase = Phase::Waiting;
    auto phase_since = clock::now();

    running = true;
    network_thread = std::thread(&Client::networkLoop, this);

    while (true) {
        input.processInputs();
        if (input.closeRequested() || input.backRequested() || server_version >= 0)
            break;

        Phase phase = game_state.getPhase();
        if (phase != last_phase) {
            last_phase = phase;
            phase_since = clock::now();
        }

        // Short grace period so a jetpack still held at death doesn't skip the results
        bool can_rematch = phase == Phase::Over && clock::now() - phase_since > std::chrono::milliseconds(1500);
        if (input.confirmPressed() && can_rematch && game_state.requestRematch())
            sendToServer(Protocol::createPacket(MSG_READY, {}));

        renderer.render(game_state, connected);
    }

    stop();
    if (input.closeRequested())
        return RunResult::Quit;
    if (server_version >= 0)
        return server_version > PROTOCOL_VERSION ? RunResult::Outdated : RunResult::HostOutdated;
    return RunResult::Menu;
}

void Client::stop()
{
    running = false;

    if (network_thread.joinable())
        network_thread.join();
}

void Client::networkLoop()
{
    std::vector<Socket::PollEntry> socket = {{client_fd}};

    while (running && connected) {
        processOutgoingMessages();

        if (Socket::poll(socket, 5) && (socket[0].readable || socket[0].closed))
            readIncomingData();
    }
}

void Client::processOutgoingMessages()
{
    std::lock_guard<std::mutex> lock(queue_mutex);

    while (!message_queue.empty()) {
        Socket::send(client_fd, message_queue.front());
        message_queue.pop();
    }
}

void Client::readIncomingData()
{
    if (!Socket::receive(client_fd, stream)) {
        DEBUG_LOG("Lost connection to server");
        connected = false;
        return;
    }

    MessageHeader header;
    std::vector<uint8_t> payload;

    while (Protocol::extractPacket(stream, header, payload))
        processMessage(header, payload);
}

void Client::processMessage(const MessageHeader &header, const std::vector<uint8_t> &payload)
{
    switch (header.type) {
        case MSG_VERSION:
            if (!payload.empty())
                server_version = payload[0];
            break;
        case MSG_MAP_DATA:
            game_state.pushMapData(payload);
            break;
        case MSG_GAME_START:
            handleGameStart(payload);
            break;
        case MSG_COUNTDOWN:
            if (!payload.empty())
                game_state.setCountdown(payload[0]);
            break;
        case MSG_GAME_STATE:
            handleGameState(payload);
            break;
        case MSG_COLLISION:
            handleCollision(payload);
            break;
        case MSG_SHOT:
            handleShot(payload);
            break;
        case MSG_ULTIMATE:
            handleUltimate(payload);
            break;
        case MSG_TITANS:
            handleTitans(payload);
            break;
        case MSG_SNIPERS:
            handleSnipers(payload);
            break;
        case MSG_BOSS:
            handleBoss(payload);
            break;
        case MSG_GAME_END:
            handleGameEnd(payload);
            break;
        default:
            break;
    }
}

void Client::handleGameStart(const std::vector<uint8_t> &payload)
{
    GameStart start;

    if (Message::decode(payload, start))
        game_state.startCountdown(start.my_number, start.roster, start.district, start.mode);
}

void Client::handleTitans(const std::vector<uint8_t> &payload)
{
    std::vector<TitanState> titans;

    if (Message::decode(payload, titans))
        game_state.setTitans(std::move(titans));
}

void Client::handleSnipers(const std::vector<uint8_t> &payload)
{
    std::vector<SniperState> snipers;

    if (Message::decode(payload, snipers))
        game_state.setSnipers(std::move(snipers));
}

void Client::handleBoss(const std::vector<uint8_t> &payload)
{
    BossState boss;

    if (Message::decode(payload, boss))
        game_state.setBoss(boss);
}

void Client::handleShot(const std::vector<uint8_t> &payload)
{
    ShotEvent shot;

    if (Message::decode(payload, shot))
        game_state.pushShot(shot);
}

void Client::handleUltimate(const std::vector<uint8_t> &payload)
{
    UltimateEvent ultimate;

    if (Message::decode(payload, ultimate))
        game_state.pushUltimate(ultimate);
}

void Client::handleGameState(const std::vector<uint8_t> &payload)
{
    std::map<int, PlayerState> players;

    if (!Message::decode(payload, players))
        return;
    for (const auto &[number, player] : players)
        game_state.updatePlayer(number, player);
}

void Client::handleCollision(const std::vector<uint8_t> &payload)
{
    CollisionEffect collision;

    if (Message::decode(payload, collision))
        game_state.handleCollision(collision.type, collision.player, collision.x, collision.y);
}

void Client::handleGameEnd(const std::vector<uint8_t> &payload)
{
    if (payload.empty())
        return;

    int winner = payload[0] == 0xFF ? -1 : payload[0];

    DEBUG_LOG(winner >= 0 ? "Game over. Player " + std::to_string(winner) + " wins!" : "Game over. No winner.");
    game_state.setWinner(winner);
}

void Client::sendToServer(const std::vector<uint8_t>& data)
{
    if (!connected || data.empty())
        return;

    std::lock_guard<std::mutex> lock(queue_mutex);

    message_queue.push(data);
}

void Client::sendAttack(bool pressed)
{
    game_state.holdAttack(pressed);
    sendToServer(Protocol::createPacket(MSG_ATTACK, {static_cast<uint8_t>(pressed)}));
}

bool Client::combatRun() const
{
    return game_state.getMode() == GameMode::Cyberpsycho;
}

bool Client::runEnded() const
{
    return !connected || game_state.getPhase() == Phase::Over;
}

void Client::answerTraumaTeam(bool call)
{
    if (game_state.answerTraumaOffer())
        sendToServer(Protocol::createPacket(MSG_TRAUMA, {static_cast<uint8_t>(call)}));
}

void Client::answerRipperdoc(int implant)
{
    if (game_state.isRipperdocOpen())
        sendToServer(Protocol::createPacket(MSG_RIPPERDOC, {static_cast<uint8_t>(implant)}));
}

bool Client::ripperdocOpen() const
{
    return game_state.isRipperdocOpen();
}

bool Client::traumaOffered() const
{
    return game_state.isTraumaOffered();
}

void Client::sendUltimate()
{
    sendToServer(Protocol::createPacket(MSG_ULTIMATE, {}));
}

void Client::sendPlayerInput(bool jet_activated)
{
    std::vector<uint8_t> payload = { static_cast<uint8_t>(jet_activated ? 1 : 0) };

    sendToServer(Protocol::createPacket(MSG_PLAYER_INPUT, payload));
}
