// What the parts of the game screen share about the run on screen: map, camera, players' motion

#ifndef RUN_HPP
    #define RUN_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <set>
#include <vector>
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
    sf::Clock shot_clock;   // since the last shot: Jinx's sprite kicks back on each bullet
};

struct Run {
    // Players are 2 tiles tall, like their server hitbox
    static constexpr float PLAYER_HEIGHT = 2.0f;

    // Rebuilt from the chunks the server streams
    Map map;
    WorldView view;
    std::map<int, int> roster;      // who plays which character
    bool cyberpsycho = false;       // the run's mode

    std::map<int, sf::Vector2f> display_positions;
    std::map<int, PlayerAnim> player_anims;
    std::map<int, sf::Clock> death_clocks;
    std::map<int, sf::Clock> attack_clocks;
    // Camera shake: seconds left, and how far it throws the view, in pixels
    float shake = 0;
    float shake_strength = 0;

    // Zappers switched off, the way the server has them: hacked for a while, or burnt for good
    struct Hack {
        sf::Clock since;
        float duration;
    };
    std::map<std::pair<int, int>, Hack> hacked_zappers;         // node -> Lucy's hack on it
    std::set<std::pair<int, int>> burnt_zappers;                // out for the rest of the run

    // Over the whole screen, for the ultimates: a flash that fades, and the ultimate's name in big letters
    struct Flash {
        sf::Color color = sf::Color::Transparent;
        float duration = 1;
        sf::Clock clock;
    };
    Flash flash;
    std::string callout;
    sf::Color callout_color;
    sf::Clock callout_clock;

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
    // The stronger of the current shake and this one: a light one doesn't cut a big one short
    void shakeAtLeast(float seconds, float strength = 12);

    // Seconds before a hacked zapper node is live again, 0 when it is
    float hackLeft(int x, int y) const;
    // The zappers in these columns go out, like on the server; only those touching rows top to bottom
    void burnZappers(float from, float to, int top = 0, int bottom = 1 << 20);
    // ... or go off for a while
    void hackZappers(float from, float to, float duration);

    void flashScreen(sf::Color color, float duration) { flash = {color, duration, sf::Clock()}; }
    void announce(const UltimateLook &look);

private:
    void smoothPositions(const std::map<int, PlayerState> &players, float dt);
    void updateSpeed(int me, float dt);
};

#endif
