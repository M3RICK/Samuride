// The players: animated runners with their jetpack exhaust, and the Attack Titan form

#ifndef PLAYERS_HPP
    #define PLAYERS_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <vector>
#include "../state.hpp"

class Canvas;
class Particles;
class Audio;
struct Sprites;
struct Run;
struct Character;
struct Animation;

class Players {
private:
    struct AnimationFrame {
        const Animation *animation;
        float time;
    };

    // The Hunter's mist: soft puffs left in the world behind him, swelling, rising and fading
    struct Puff {
        sf::Vector2f pos;   // in tiles
        sf::Color color;
        sf::Clock age;
    };
    static constexpr float PUFF_LIFE = 0.7f;
    static constexpr float PUFF_GAP = 1 / 30.f;     // seconds between two: as thick at 144 fps as at 30

    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    Particles &particles;
    std::vector<Puff> mist;
    sf::Clock puff_clock;

    void drawMist();
    void drawMistGhosts(const sf::Sprite &sprite, sf::Color color, float tile_size);
    void emitExhaust(const std::map<int, PlayerState> &players);
    bool updateAirState(int player_num);
    AnimationFrame pickAnimation(int player_num, const PlayerState &player, bool on_ground);
    void drawPlayer(int player_num, const PlayerState &player, bool is_me);
    void updateSprint(int player_num, const PlayerState &player);
    void drawAfterimages(const sf::Sprite &sprite, float tile_size);
    void drawShieldBubble(sf::Vector2f center, float tile_size);

    // The Attack Titan: forms come from the server, how long a player has been in one is timed here
    void updateForm(int player_num, const PlayerState &player, bool is_me);
    void drawShift(int player_num, const PlayerState &player, bool is_me);
    void drawSteamPuff(int player_num);
    void drawShiftFrame(const Character &sheet, const Animation &anim, int frame, float center_x, uint8_t alpha,
                        bool flip = false);

public:
    Players(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio, Particles &particles);

    // `live`: the run is on, jetpacks smoke
    void draw(const std::map<int, PlayerState> &players, int me, bool live);
};

#endif
