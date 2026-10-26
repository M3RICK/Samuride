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
    sendConnectMessage();
    return true;
}

// [runner][district][mode][upgrade levels...]; when joining, the host's district and mode apply
void Client::sendConnectMessage()
{
    bool joining = loadout.district < 0 || loadout.mode < 0;
    std::vector<uint8_t> payload = {static_cast<uint8_t>(loadout.character),
                                    joining ? NOT_PICKED : static_cast<uint8_t>(loadout.district),
                                    joining ? NOT_PICKED : static_cast<uint8_t>(loadout.mode)};

    for (int level : loadout.upgrades)
        payload.push_back(level);
    sendToServer(Protocol::createPacket(MSG_CONNECT, payload));
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
        if (input.closeRequested() || input.backRequested())
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
    return input.closeRequested() ? RunResult::Quit : RunResult::Menu;
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

// [my number][player count]([number][character])...[district][mode]
void Client::handleGameStart(const std::vector<uint8_t> &payload)
{
    std::map<int, int> roster;

    if (payload.size() < 2)
        return;

    size_t count = payload[1];
    for (size_t i = 0; i < count && 3 + 2 * i < payload.size(); i++)
        roster[payload[2 + 2 * i]] = payload[3 + 2 * i];

    size_t district_pos = 2 + 2 * count;
    int district = district_pos < payload.size() ? payload[district_pos] : 0;
    auto mode = static_cast<GameMode>(district_pos + 1 < payload.size() ? payload[district_pos + 1] : 0);
    game_state.startCountdown(payload[0], roster, district, mode);
}

// [count]([type][x in 1/100 tile, 4 bytes])...
void Client::handleTitans(const std::vector<uint8_t> &payload)
{
    std::vector<TitanState> titans;

    for (size_t pos = 1; !payload.empty() && pos + 5 <= payload.size(); pos += 5) {
        auto type = static_cast<TitanType>(payload[pos]);
        titans.push_back({type, Protocol::readU32(payload.data() + pos + 1) / POSITION_SCALE});
    }
    game_state.setTitans(std::move(titans));
}

// [count]([phase][y in 1/100 tile, 2 bytes])...
void Client::handleSnipers(const std::vector<uint8_t> &payload)
{
    std::vector<SniperState> snipers;

    for (size_t pos = 1; !payload.empty() && pos + 3 <= payload.size(); pos += 3)
        snipers.push_back({static_cast<SniperPhase>(payload[pos]), Protocol::readU16(payload.data() + pos + 1) / POSITION_SCALE});
    game_state.setSnipers(std::move(snipers));
}

// [fighting][health %][attack][phase][x, 4 bytes][rockets' x, 4 bytes][y, 2 bytes][seconds into the phase, 2 bytes]
void Client::handleBoss(const std::vector<uint8_t> &payload)
{
    if (payload.size() < 16)
        return;
    const uint8_t *p = payload.data();
    game_state.setBoss({p[0] != 0, p[1], static_cast<BossAttack>(p[2]), static_cast<BossPhase>(p[3]),
                        Protocol::readU32(p + 4) / POSITION_SCALE, Protocol::readU32(p + 8) / POSITION_SCALE,
                        Protocol::readU16(p + 12) / POSITION_SCALE, Protocol::readU16(p + 14) / POSITION_SCALE});
}

void Client::handleShot(const std::vector<uint8_t> &payload)
{
    if (payload.size() >= 2)
        game_state.pushShot({payload[0], static_cast<Weapon>(payload[1])});
}

// [player][ultimate][x in 1/100 tile, 4 bytes][y in 1/100 tile, 2 bytes]
void Client::handleUltimate(const std::vector<uint8_t> &payload)
{
    if (payload.size() >= 8)
        game_state.pushUltimate({payload[0], static_cast<Ultimate>(payload[1]),
                                 Protocol::readU32(payload.data() + 2) / POSITION_SCALE,
                                 Protocol::readU16(payload.data() + 6) / POSITION_SCALE});
}

void Client::handleGameState(const std::vector<uint8_t> &payload)
{
    for (size_t pos = 0; pos + 13 <= payload.size(); pos += 13) {
        const uint8_t *p = payload.data() + pos;

        // docs/PROTOCOL.md, MSG_GAME_STATE
        game_state.updatePlayer(p[0], {
            .x = Protocol::readU32(p + 1) / POSITION_SCALE,
            .y = Protocol::readU16(p + 5) / POSITION_SCALE,
            .score = Protocol::readU16(p + 7),
            .jet_active = (p[9] & 1) != 0,
            .form = static_cast<Form>((p[9] >> 1) & 3),
            .shielded = (p[9] & 8) != 0,
            .sprinting = (p[9] & 16) != 0,
            .ram = (p[9] >> 5) & 3,
            .berserk = (p[9] & 128) != 0,
            .ultimate = p[10],
            .spears = p[11],
            .grace = (p[12] & 1) != 0,
            .style = (p[12] >> 1) & 7,
        });
    }
}

void Client::handleCollision(const std::vector<uint8_t> &payload)
{
    if (payload.size() < 8)
        return;

    game_state.handleCollision(payload[0], payload[1],
        Protocol::readU32(payload.data() + 2), Protocol::readU16(payload.data() + 6));
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

void Client::sendUltimate()
{
    sendToServer(Protocol::createPacket(MSG_ULTIMATE, {}));
}

void Client::sendPlayerInput(bool jet_activated)
{
    std::vector<uint8_t> payload = { static_cast<uint8_t>(jet_activated ? 1 : 0) };

    sendToServer(Protocol::createPacket(MSG_PLAYER_INPUT, payload));
}
