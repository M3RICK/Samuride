// What the parts of the game screen share about the run on screen: map, camera, players' motion

#ifndef RUN_HPP
    #define RUN_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include "canvas.hpp"
#include "../state.hpp"
#include "../../common/map.hpp"

struct PlayerAnim {
    bool on_ground = true;
    sf::Clock air_clock;
    Form form = Form::Normal;
    sf::Clock form_clock;   // time in the current form
    float smoke_x = 0;      // where the titan stood when it ended, in tiles
    bool sprinting = false;
    sf::Clock sprint_clock; // time since the Sandevistan kicked in
    bool berserk = false;
    sf::Clock berserk_clock;
};

struct Run {
    // Players are 2 tiles tall, like their server hitbox
    static constexpr float PLAYER_HEIGHT = 2.0f;

    // Rebuilt from the chunks the server streams
    Map map;
    WorldView view;
    std::map<int, int> roster;      // who plays which character

    std::map<int, sf::Vector2f> display_positions;
    std::map<int, PlayerAnim> player_anims;
    std::map<int, sf::Clock> death_clocks;
    std::map<int, sf::Clock> attack_clocks;
    std::map<std::pair<int, int>, sf::Clock> hacked_zappers;    // node -> since Lucy hacked it
    float shake = 0;

    // Speed feel: measured from your own movement, no extra network data
    float run_speed = 0;    // tiles per second
    float last_x = -1;      // your position on the previous frame, -1 before the first one
    float run_phase = 0;    // run animation clock, ticking faster as you speed up

    // New run (first one or rematch): forget the previous one, keep the map
    void reset();
    // Eases the players toward the server's positions, then the camera and the speed after yours
    void follow(const std::map<int, PlayerState> &players, int me, float dt);

    sf::Vector2f toScreen(sf::Vector2f world) const { return view.toScreen(world); }
    float floor() const { return map.getHeight(); }
    int characterOf(int player_num) const;
    // Seconds before a hacked zapper node is live again, 0 when it is
    float hackLeft(int x, int y) const;
    void shakeAtLeast(float amount);

private:
    void smoothPositions(const std::map<int, PlayerState> &players, float dt);
    void updateSpeed(int me, float dt);
};

#endif
