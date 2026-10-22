// CYBERPSYCHO ultimates: firing one, and what each destroys (common/ultimates.hpp has their numbers).
// The client draws them in render/ultimates.cpp.

#include "server.hpp"
#include <algorithm>

// Only with a full gauge, out of the Attack Titan's forms. While Mikasa has thunder spears, the key throws one.
void Server::handleUltimate(int client_fd)
{
    auto it = players.find(client_fd);

    if (mode != GameMode::Cyberpsycho || phase != Phase::Running || it == players.end())
        return;

    Player &player = it->second;
    Ultimate ultimate = ultimateOf(player.getCharacter());

    if (player.getSpears() > 0) {
        if (player.throwSpear())
            fireProjectile(client_fd, player, Weapon::Spear);
        return;
    }
    if (ultimate == Ultimate::None || !player.ultimateReady() || player.getForm() != Form::Normal)
        return;
    player.spendUltimate();

    float start = player.getX() + 1;
    float line = player.getY() + Player::HEIGHT / 2;
    std::vector<uint8_t> payload = {static_cast<uint8_t>(player.getPlayerNumber()), static_cast<uint8_t>(ultimate)};
    Protocol::pushU32(payload, start * POSITION_SCALE);
    Protocol::pushU16(payload, line * POSITION_SCALE);
    broadcastToAllClients(Protocol::createPacket(MSG_ULTIMATE, payload));

    // A new ultimate starts here (and is drawn in the client's render/ultimates.cpp)
    switch (ultimate) {
        case Ultimate::FireWave:
            sweeps.push_back({client_fd, start, start, FIRE_WAVE_RANGE, FIRE_WAVE_SPEED, 0, 1 << 20});
            break;
        case Ultimate::Fishbones: {
            auto [top, bottom] = fishbonesRows(line);
            sweeps.push_back({client_fd, start, start, FISHBONES_RANGE, FISHBONES_SPEED, top, bottom,
                              FISHBONES_CHARGE_TIME});
            player.startFishbones();
            break;
        }
        case Ultimate::Moonlight: {
            auto [top, bottom] = moonlightRows(line);
            sweeps.push_back({client_fd, start, start, MOONLIGHT_RANGE, MOONLIGHT_SPEED, top, bottom});
            break;
        }
        case Ultimate::Nuke:
            nukes.push_back({client_fd, start + NUKE_DISTANCE, NUKE_DELAY});
            break;
        case Ultimate::Breach:
            breach(client_fd, start);
            break;
        case Ultimate::ThunderSpears:
            player.giveSpears();    // thrown with this same key, see the top of this function
            break;
        case Ultimate::None:
            break;
    }
}

// The fire wave, Fishbones and the Moonlight Blade sweep on, the nukes land
void Server::updateUltimates()
{
    for (Sweep &sweep : sweeps) {
        if (sweep.delay > 0) {
            sweep.delay -= TICK_DT;
            continue;
        }
        float front = std::min(sweep.start + sweep.range, sweep.front + sweep.speed * TICK_DT);
        scorch(sweep.owner_fd, sweep.front, front, sweep.start - 1, sweep.top, sweep.bottom);
        sweep.front = front;
    }
    std::erase_if(sweeps, [](const Sweep &sweep) { return sweep.front >= sweep.start + sweep.range; });

    for (Nuke &nuke : nukes) {
        nuke.timer -= TICK_DT;
        if (nuke.timer <= 0)
            scorch(nuke.owner_fd, nuke.impact - NUKE_RADIUS, nuke.impact + NUKE_RADIUS, nuke.impact - NUKE_RADIUS);
    }
    std::erase_if(nukes, [](const Nuke &nuke) { return nuke.timer <= 0; });
}

// Every column from `from` to `to` burns, rows top to bottom (floor to ceiling by default): drones go
// down, zappers touching those rows go out for the rest of the run, and titans between `titans_from`
// and `to` die if the rows reach them. The owner gets the bounties.
void Server::scorch(int owner_fd, float from, float to, float titans_from, int top, int bottom)
{
    int height = game_map.getHeight();
    auto owner = players.find(owner_fd);

    bottom = std::min(bottom, height - 1);
    for (int x = std::max(0, static_cast<int>(from)); x <= static_cast<int>(to); x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(owner_fd, x, y);
            } else if (game_map.getTile(x, y) == ZAPPER_TILE && !hacked_zappers.count({x, y})) {
                for (std::pair<int, int> node : zapperNodes(game_map, x, y))
                    hacked_zappers[node] = 1e9f;
            }
        }
    }
    std::erase_if(titans, [&](const Titan &titan) {
        bool reached = bottom + 1 > height - titanStats(titan.type).height;
        if (titan.x > to || titan.x < titans_from || !reached || owner == players.end())
            return false;
        owner->second.addScore(TITAN_BOUNTY);
        notifyCollision(TITAN_KILL, owner->second, titan.x, height - 1);
        return true;
    });
}

// From just behind her to the end of the screen: drones go down, zappers go off for BREACH_TIME
// (the burnt ones stay out)
void Server::breach(int owner_fd, float start)
{
    for (int x = std::max(0, static_cast<int>(start - 2)); x <= static_cast<int>(start + BREACH_AHEAD); x++) {
        for (int y = 0; y < static_cast<int>(game_map.getHeight()); y++) {
            if (isLiveDrone(x, y)) {
                destroyDrone(owner_fd, x, y);
            } else if (game_map.getTile(x, y) == ZAPPER_TILE) {
                float &left = hacked_zappers[{x, y}];
                left = std::max(left, BREACH_TIME);
            }
        }
    }
}
