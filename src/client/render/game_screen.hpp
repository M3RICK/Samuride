// The run on screen: world, effects and HUD, then the game over screens

#ifndef GAME_SCREEN_HPP
    #define GAME_SCREEN_HPP

#include <map>
#include <string>
#include <vector>
#include "run.hpp"
#include "particles.hpp"
#include "tiles.hpp"
#include "players.hpp"
#include "titans.hpp"
#include "shots.hpp"

class Audio;
class Backdrops;

class GameScreen {
private:
    struct ActiveEffect {
        CollisionEffect effect;
        sf::Clock clock;
        bool mine;      // caused by this player
    };

    // Screen-space streak sold as wind: more and faster as the run speeds up
    struct SpeedLine {
        sf::Vector2f pos;
        float length;
        float speed;    // pixels per second
    };

    static constexpr float BANNER_DURATION = 2.5f;
    static constexpr float EFFECT_DURATION = 0.6f;
    static constexpr float GAME_OVER_DELAY = 0.9f;
    static constexpr float PROMPT_DELAY = 1.5f;

    Canvas &canvas;
    const Sprites &sprites;
    const Backdrops &backdrops;
    Audio &audio;

    Run run;
    Particles particles;
    Tiles tiles;
    Players runners;
    Titans titans;
    Shots shots;

    std::vector<SpeedLine> speed_lines;
    std::vector<ActiveEffect> effects;
    int drones_down = 0;
    int heard_countdown = -1;   // last countdown value beeped

    // District banner ("ENTERING ..."), shown when the scenery changes
    int shown_backdrop = -1;
    std::string banner;
    std::string banner_tagline;
    sf::Clock banner_clock;

    int best_distance = 0;
    bool new_record = false;
    int wallet = 0;     // eddies saved, once this run's are in

    Phase last_phase = Phase::Waiting;
    sf::Clock phase_clock;

    // game_screen.cpp: the run
    void onPhaseChange(GameState &state, Phase phase);
    void resetRun();
    void recordSoloDistance(GameState &state);
    void bankEddies(GameState &state);
    void update(GameState &state, const std::map<int, PlayerState> &players, int me, float dt);
    void updateSounds(GameState &state, Phase phase, const std::map<int, PlayerState> &players, int me);
    void drawWorld(GameState &state, const std::map<int, PlayerState> &players, int me, float dt);
    void drawHackTarget(const std::map<int, PlayerState> &players, int me, const std::set<std::pair<int, int>> &destroyed);
    void drawDistrict(int district);
    void updateSpeedLines(float dt);
    void drawSpeedLines();
    void addEffect(const CollisionEffect &effect, int me);
    void drawEffects();

    // game_hud.cpp: what is drawn over the world
    void drawHUD(const std::map<int, PlayerState> &players, int me, bool cyberpsycho);
    void drawHUDLine(int player_num, const PlayerState &player, int me, float y, bool cyberpsycho);
    void drawPowerUps(const PlayerState &player, int me);
    void drawGauge(const std::string &label, sf::Color color, float left, float y);
    void drawBanner();
    void drawCountdown(Phase phase, int countdown);
    void drawGameOver(GameState &state, const std::map<int, PlayerState> &players, int me);
    void drawSoloResults(const PlayerState &me);
    void drawVersusResults(const std::map<int, PlayerState> &players, int winner, int me);
    void drawRematchPrompt(GameState &state, bool solo);
    void drawConnectionLost();

public:
    GameScreen(Canvas &canvas, const Sprites &sprites, const Backdrops &backdrops, Audio &audio);

    // Keeps up with the game even while waiting in the lobby, where nothing is drawn but a lost connection
    void draw(GameState &state, Phase phase, bool connected, float dt);
};

#endif
