// What the parts of the game screen share about the run on screen: map, camera, players' motion

#include "run.hpp"
#include <algorithm>

void Run::reset()
{
    display_positions.clear();
    player_anims.clear();
    death_clocks.clear();
    attack_clocks.clear();
    hacked_zappers.clear();
    fire_waves.clear();
    nukes.clear();
    rockets.clear();
    breaches.clear();
    flash = {};
    callout.clear();
    burnt_zappers.clear();
    shake = 0;
    run_speed = 0;
    last_x = -1;
}

void Run::follow(const std::map<int, PlayerState> &players, int me, float dt)
{
    view.tile_size = (Screen::PLAY_BOTTOM - Screen::PLAY_TOP) / map.getHeight();
    smoothPositions(players, dt);

    auto mine = display_positions.find(me);
    if (mine != display_positions.end())
        view.camera_x = std::max(0.0f, mine->second.x - Screen::WIDTH / view.tile_size / 4);
    updateSpeed(me, dt);
}

// The server ticks at 60Hz, easing toward its positions hides the jitter
void Run::smoothPositions(const std::map<int, PlayerState> &players, float dt)
{
    for (const auto &[player_num, player] : players) {
        sf::Vector2f target(player.x, player.y);
        auto [it, inserted] = display_positions.try_emplace(player_num, target);
        sf::Vector2f &pos = it->second;

        if ((target - pos).length() > 3)
            pos = target;
        else
            pos += (target - pos) * std::min(1.0f, dt * 20);
    }
}

// Your speed from how far you moved since the last frame, eased so it doesn't flicker
void Run::updateSpeed(int me, float dt)
{
    auto mine = display_positions.find(me);

    if (mine == display_positions.end() || dt <= 0)
        return;

    float measured = last_x < 0 ? 0 : (mine->second.x - last_x) / dt;
    last_x = mine->second.x;
    if (measured >= 0 && measured < 40)
        run_speed += (measured - run_speed) * std::min(1.0f, dt * 2);
    run_phase += dt * std::max(1.0f, run_speed / 9);
}

// Unknown players play the character matching their number
int Run::characterOf(int player_num) const
{
    auto it = roster.find(player_num);

    return it != roster.end() ? it->second : player_num;
}

void Run::shakeAtLeast(float amount)
{
    shake = std::max(shake, amount);
}

float Run::hackLeft(int x, int y) const
{
    auto hacked = hacked_zappers.find({x, y});

    if (hacked == hacked_zappers.end())
        return 0;
    return std::max(0.0f, hacked->second.duration - hacked->second.since.getElapsedTime().asSeconds());
}

void Run::burnZappers(float from, float to, int top, int bottom)
{
    bottom = std::min(bottom, static_cast<int>(map.getHeight()) - 1);
    for (int x = std::max(0, static_cast<int>(from)); x <= static_cast<int>(to); x++) {
        for (int y = std::max(0, top); y <= bottom; y++) {
            if (map.getTile(x, y) == ZAPPER_TILE && !burnt_zappers.count({x, y})) {
                for (std::pair<int, int> node : zapperNodes(map, x, y))
                    burnt_zappers.insert(node);
            }
        }
    }
}

void Run::hackZappers(float from, float to, float duration)
{
    for (int x = std::max(0, static_cast<int>(from)); x <= static_cast<int>(to); x++) {
        for (int y = 0; y < static_cast<int>(map.getHeight()); y++) {
            if (map.getTile(x, y) == ZAPPER_TILE && hackLeft(x, y) < duration)
                hacked_zappers[{x, y}] = {sf::Clock(), duration};
        }
    }
}

void Run::announce(const UltimateLook &look)
{
    callout = look.name;
    callout_color = look.color;
    callout_clock.restart();
}
