// The players: animated runners with their jetpack exhaust, and the Attack Titan form

#ifndef PLAYERS_HPP
    #define PLAYERS_HPP

#include <map>
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

    Canvas &canvas;
    const Sprites &sprites;
    Run &run;
    Audio &audio;
    Particles &particles;

    void emitExhaust(const std::map<int, PlayerState> &players);
    bool updateAirState(int player_num);
    AnimationFrame pickAnimation(int player_num, const PlayerState &player, bool on_ground);
    void drawPlayer(int player_num, const PlayerState &player, bool is_me);

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
