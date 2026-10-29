// The run on screen: world, effects and HUD, then the game over screens

#ifndef GAME_SCREEN_HPP
    #define GAME_SCREEN_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include "run.hpp"
#include "../wallet.hpp"
#include "particles.hpp"
#include "tiles.hpp"
#include "players.hpp"
#include "titans.hpp"
#include "snipers.hpp"
#include "boss.hpp"
#include "shots.hpp"
#include "ultimates.hpp"
#include "../audio.hpp"
#include "../settings.hpp"

class Backdrops;

class GameScreen {
private:
    struct ActiveEffect {
        CollisionEffect effect;
        sf::Clock clock;
        bool mine;      // caused by this player
        int bounty = 0; // what it paid you, for the "+N"
    };

    // Screen-space streak sold as wind: more and faster as the run speeds up
    struct SpeedLine {
        sf::Vector2f pos;
        float length;
        float speed;    // pixels per second
    };

    static constexpr float BANNER_DURATION = 2.5f;
    static constexpr float EFFECT_DURATION = 0.6f;
    static constexpr float COIN_FLY_SHARE = 0.4f;  // of the effect, for an eddy to reach its taker
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
    Snipers snipers;
    Smasher smasher;
    Shots shots;
    Ultimates ultimates;

    std::vector<SpeedLine> speed_lines;
    std::vector<ActiveEffect> effects;
    int drones_down = 0;
    // Mikasa's blade biting: the world freezes for a blink (hit-stop) and the cut shows on the drone
    static constexpr float HIT_STOP = 0.035f;   // felt, not seen: longer breaks the run's rhythm
    static constexpr float CUT_TIME = 0.25f;
    float hit_stop = 0;         // seconds of freeze left
    struct Cut {
        sf::Vector2f pos;       // in tiles
        sf::Clock age;
    };
    std::vector<Cut> cuts;
    // Lucy's hack: a crackling bolt from her to the drone, and from it to the one the hack jumps to
    static constexpr float ZAP_TIME = 0.2f;
    struct Zap {
        sf::Vector2f from, to;  // in tiles
        sf::Clock age;
    };
    std::vector<Zap> zaps;
    std::set<int> bounced;      // players whose next drone down is the one their hack jumped to
    int coin_streak = 0;        // eddies taken in a row, shown as one "+N"
    sf::Clock coin_clock;       // since the last one
    sf::Vector2f last_coin;     // where it was, in tiles
    std::map<int, sf::Vector2f> last_drone;     // each player's last drone down, where Lucy's hack bounces from
    int heard_countdown = -1;   // last countdown value beeped

    // District banner ("ENTERING ..."), shown when the scenery changes
    int shown_backdrop = -1;
    std::string banner;
    std::string banner_tagline;
    sf::Clock banner_clock;

    Bests bests;
    bool cyberpsycho_run = false;   // which of the bests this run chases
    bool new_record = false;
    bool new_drone_record = false;
    int wallet = 0;     // eddies saved, once this run's are in
    std::string ultimate_key;   // its name, read from the settings at the start of each run
    bool ultimate_was_ready = false;
    int spears_shown = 0;       // to see one thrown
    int style_shown = 0;        // your style rank last frame (Dante), to hear and see it go up
    sf::Clock style_clock;      // since it last went up
    // The HUD's face reacts for a moment to what happens to you, like Doom's. In order: a mood only
    // interrupts a lesser one.
    enum class Mood { Idle, Pickup, Kill, Frenzy, Surprised, Shout };
    Mood mood = Mood::Idle;
    sf::Clock mood_clock;
    int kill_streak = 0;        // kills in a quick row
    // Idle, the head turns a step at a time toward a column of the faces' first row, holds, turns back
    int head = 0;
    int head_target = 0;
    float head_moved = 0;       // canvas time of its last step
    float head_until = 0;       // when it picks a new target
    bool titan_coming = false;  // one is announced on the right
    float sandevistan_time = SANDEVISTAN_TIME;  // longer with the upgrade, read at the start of each run
    float titan_time = TITAN_TIME;              // the same with the Titan Serum
    UpgradeLevels upgrades{};                   // yours, read at the start of each run
    bool trauma_ready = false;                  // the Trauma Team can still come this run
    sf::Clock trauma_offer_clock;               // since it asked whether to come
    char death_cause = 0;                       // what downed you (a MSG_COLLISION type), 0 while alive
    Wallet::Goal next_goal;                     // what the eddies buy next, once this run's are in

    Phase last_phase = Phase::Waiting;
    sf::Clock phase_clock;

    // game_screen.cpp: the run
    void onPhaseChange(GameState &state, Phase phase);
    void resetRun();
    void recordSoloBests(GameState &state);
    void bankEddies(GameState &state);
    void update(GameState &state, const std::map<int, PlayerState> &players, int me, float dt);
    void updateSounds(GameState &state, Phase phase, const std::map<int, PlayerState> &players, int me);
    void drawWorld(GameState &state, const std::map<int, PlayerState> &players, int me, float dt);
    void drawHackTarget(const std::map<int, PlayerState> &players, int me, const std::set<std::pair<int, int>> &destroyed);
    void drawBlinkTarget(const std::map<int, PlayerState> &players, int me, float held);
    void drawDistrict(int district);
    void updateSpeedLines(float dt);
    void drawSpeedLines();
    // Hit effects (game_effects.cpp)
    void addEffect(const CollisionEffect &effect, int me);
    void playerDown(const CollisionEffect &effect, Audio::Sfx sound, sf::Color color, float shake, float strength);
    void titanDown(const CollisionEffect &effect, bool mine);
    void bossDown(const CollisionEffect &effect, bool mine);
    void droneDown(const CollisionEffect &effect, int me);
    void hackBounced(const CollisionEffect &effect);
    void spearBlast(const CollisionEffect &effect);
    void zapperHacked(const CollisionEffect &effect);
    void shieldBroken(const CollisionEffect &effect, bool mine);
    void traumaRescue(const CollisionEffect &effect, bool mine);
    void drawTraumaTeam();
    void drawLook(const Character &look, const Animation &anim, int frame, sf::Vector2f feet, float height);
    void jackpot(const CollisionEffect &effect, bool mine);
    void pinataBurst(const CollisionEffect &effect);
    void powerUpTaken(const CollisionEffect &effect, bool mine);
    void eddyTaken(const CollisionEffect &effect);
    void drawEffects();
    void drawCuts();
    void drawZaps();

    // game_hud.cpp: what is drawn over the world
    void drawHUD(const std::map<int, PlayerState> &players, int me, bool cyberpsycho);
    void drawHUDLine(int player_num, const PlayerState &player, int me, float y);
    void drawPowerUps(const PlayerState &player, int me);
    void drawGauge(const std::string &label, sf::Color color, float left, float y);
    void drawRunnerPanel(const PlayerState &player, int me, bool ultimate);
    void drawStyle(const PlayerState &player);
    void drawRing(sf::Vector2f center, float radius, int segments, float lit, sf::Color color, bool pulsing);
    void drawFace(const PlayerState &player, int me, sf::Vector2f center, float radius);
    sf::Vector2i faceCell(const PlayerState &player, int me);
    void react(Mood new_mood);
    void drawTouchButtons(const PlayerState &player, int me);
    void drawMenuButton();
    void drawTraumaOffer(const PlayerState &me);
    void drawFlashAndCallout();
    void drawBanner();
    void drawCountdown(Phase phase, int countdown);
    void drawLoadout();
    void drawDeathCause(sf::Vector2f pos);
    void drawNextGoal(sf::Vector2f pos);
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
