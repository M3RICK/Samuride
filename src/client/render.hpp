/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef RENDER_HPP
#define RENDER_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "../common/map.hpp"
#include "state.hpp"
#include "characters.hpp"
#include "audio.hpp"

struct MenuState;

// Uniform random float in [min, max]
float randf(float min, float max);

// Split by theme over four files:
//   render.cpp        window, assets, frame lifecycle, text helpers
//   render_menu.cpp   menu, character and district select, lobby
//   render_world.cpp  backdrops, map tiles, players
//   render_game.cpp   game screens, effects, particles, HUD
class Renderer {
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

    // CYBERPSYCHO projectile, drawn from its shooter and its age
    struct ShotTrail {
        int player;
        Weapon weapon;
        sf::Clock age;
    };

    // World space, in tiles
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life;
        float max_life;
        float size;
        float gravity;
        sf::Color color;
    };

    // A district's look: one image cut into horizontal layers, each scrolling at its own speed.
    // A layer can also chain several pictures one after the other, to avoid looping too soon.
    struct Backdrop {
        struct Segment {
            const sf::Texture *texture;
            sf::IntRect source;
        };

        struct Layer {
            float top;          // fractions of the main image height
            float bottom;
            float parallax;     // 1 moves with the map
            std::vector<Segment> segments;  // empty: the band of the main image
        };

        std::string name;
        std::string tagline;
        const sf::Texture *texture;     // main image: calibration and layers without segments
        // Calibration knobs: image height fractions landing on the top of the map and on the feet line
        float ceiling;
        float floor;
        sf::Color tint;
        std::vector<Layer> layers;
    };

    struct PlayerAnim {
        bool on_ground = true;
        sf::Clock air_clock;
    };

    struct AnimationFrame {
        const Animation *animation;
        float time;
    };

    sf::RenderWindow window;

    // Game assets
    sf::Texture waiting_screen_texture;
    sf::Texture coin_texture;
    sf::Texture electric_texture;
    sf::Texture drone_texture;
    sf::Font font;
    // The jetpack font has no punctuation: text that needs some falls back to this one
    sf::Font fallback_font;
    std::string missing_glyphs;

    std::vector<Backdrop> backdrops;
    std::map<std::string, sf::Texture> textures;   // shared between backdrops, loaded once

    // District banner ("ENTERING ..."), shown when the scenery changes
    int shown_backdrop;
    std::string banner;
    std::string banner_tagline;
    sf::Clock banner_clock;

    // One per CHARACTER_DIRS entry, and who plays which in the current run
    std::vector<Character> characters;
    std::map<int, int> roster;

    // One per TitanType, and how many were on screen last frame (a new one stomps in)
    std::vector<Character> titan_sprites;
    size_t titans_on_screen;

    Audio audio;
    int heard_countdown;    // last countdown value beeped

    // The run, rebuilt from the chunks the server streams
    Map world_map;
    std::string lobby_info;

    // Camera & world, in tiles
    float camera_x;
    float tile_size;
    std::map<int, sf::Vector2f> display_positions;

    // Speed feel: measured from your own movement, no extra network data
    float run_speed;        // tiles per second
    float last_x;           // your position on the previous frame, -1 before the first one
    float run_phase;        // run animation clock, ticking faster as you speed up
    std::vector<SpeedLine> speed_lines;

    std::vector<ActiveEffect> effects;
    std::vector<Particle> particles;
    std::map<int, PlayerAnim> player_anims;
    std::map<int, sf::Clock> death_clocks;
    float shake;

    // CYBERPSYCHO
    std::vector<ShotTrail> shots;
    std::map<int, sf::Clock> attack_clocks;
    int drones_down;

    int best_distance;
    bool new_record;

    Phase last_phase;
    sf::Clock phase_clock;
    sf::Clock frame_clock;
    sf::Clock animation_clock;

    sf::VertexArray scanlines;

    //===========================================================================
    // Render parameters
    //===========================================================================
    static constexpr float SCREEN_WIDTH = 1920;
    static constexpr float SCREEN_HEIGHT = 1080;
    static constexpr sf::Vector2f SCREEN_CENTER{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};

    // The map is stretched between these two screen lines
    static constexpr float PLAY_TOP = 40;
    static constexpr float PLAY_BOTTOM = 1000;

    // BOTH WORLDS: the districts take turns every ZONE_LENGTH metres, crossfading over ZONE_FADE
    static constexpr float ZONE_LENGTH = 250;
    static constexpr float ZONE_FADE = 25;
    static constexpr float BANNER_DURATION = 2.5f;

    // Players are 2 tiles tall, like their server hitbox
    static constexpr float PLAYER_HEIGHT = 2.0f;

    // The background doesn't tile seamlessly, copies crossfade over this share of its width
    static constexpr float BG_OVERLAP = 0.12f;
    static constexpr float EFFECT_DURATION = 0.6f;
    static constexpr float GAME_OVER_DELAY = 0.9f;
    static constexpr float PROMPT_DELAY = 1.5f;
    static constexpr int DRONE_FRAMES = 4;

    // Night City palette
    static constexpr sf::Color NEON_YELLOW{252, 238, 10};
    static constexpr sf::Color NEON_CYAN{0, 240, 255};
    static constexpr sf::Color NEON_RED{255, 0, 60};
    static constexpr sf::Color NEON_PINK{255, 40, 200};
    static constexpr sf::Color NEON_ORANGE{255, 140, 0};
    static constexpr sf::Color DIM_WHITE{200, 200, 200};
    static constexpr sf::Color HINT_GREY{150, 150, 150};
    static constexpr sf::Color FADED{255, 255, 255, 130};   // unselected cards

    //===========================================================================
    // render.cpp: window, assets, frame lifecycle, text helpers
    //===========================================================================
    void createWindow();
    void loadAssets();
    void loadBackdrops();
    void loadCharacters();
    void loadTitans();
    void loadFonts();
    void loadTexture(sf::Texture &texture, const std::string &path);
    const sf::Texture *sharedTexture(const std::string &path);

    void beginFrame(bool in_game);
    void endFrame();
    void onPhaseChange(GameState &state, Phase phase);
    void resetRun();
    void recordSoloDistance(GameState &state);

    bool blink() const;
    sf::Text makeText(const std::string &content, unsigned int size, sf::Color color,
                      sf::Vector2f pos, sf::Vector2f anchor = {0.5f, 0.5f});
    void drawGlitchText(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos);
    static sf::Color withAlpha(sf::Color color, uint8_t alpha);
    sf::View screenView() const;
    void drawOverlay(uint8_t alpha);
    void drawCard(sf::FloatRect box, bool selected);
    void drawHalo(sf::Vector2f center, float radius, sf::Color color);
    void drawBacking(sf::Vector2f center, float radius, sf::Color ring);
    void drawImage(const sf::Texture &texture, sf::Vector2f pos, float height, sf::Vector2f anchor, sf::Color color);
    std::string playerLabel(int player_num, int me) const;
    const Character &characterOf(int player_num) const;

    //===========================================================================
    // render_menu.cpp: menu, character and district select, lobby
    //===========================================================================
    void renderMenuBackground(const MenuState &menu);
    void renderTagline();
    void renderMainMenu(const MenuState &menu);
    void renderMenuItem(int index, bool selected);
    void renderJoinField(const MenuState &menu);
    void renderCharacterSelect(const MenuState &menu);
    void drawCharacterCard(int index, bool selected, float time);
    void drawCharacterPreview(const Character &character, sf::FloatRect box, bool selected, float time);
    void renderOptions(const MenuState &menu);
    void drawOptionRow(int index, const MenuState &menu);
    void drawVolumeBar(int volume, sf::Vector2f right, bool selected);
    void renderModeSelect(const MenuState &menu);
    void drawModeCard(int index, bool selected);
    void renderDistrictSelect(const MenuState &menu);
    void drawDistrictCard(int index, bool selected);
    std::vector<Backdrop::Segment> districtSlices(int district) const;
    Backdrop::Segment thumbnailOf(const Backdrop &backdrop, size_t index) const;
    void drawThumbnail(const Backdrop::Segment &segment, sf::FloatRect dest, float from, float to);
    void renderPoster();
    void renderWaitingScreen();

    //===========================================================================
    // render_world.cpp: backdrops, map tiles, players
    //===========================================================================
    sf::Vector2f toScreen(sf::Vector2f world) const;
    void smoothPositions(const std::map<int, PlayerState> &players, float dt);
    void updateCamera(int me);
    void updateSpeed(int me, float dt);
    void updateSpeedLines(float dt);
    void renderSpeedLines();

    void renderDistrict(int district);
    void renderBackdrop(const Backdrop &backdrop, float opacity);
    void renderLayer(const Backdrop &backdrop, const Backdrop::Layer &layer, float y0, float y1, sf::Color color);
    void drawFadingPicture(const Backdrop::Segment &segment, sf::FloatRect dest, float fade, sf::Color color);

    void renderMapTiles(const std::set<std::pair<int, int>> &collected, const std::set<std::pair<int, int>> &destroyed);
    void renderCoin(int x, int y, float time);
    void renderZapper(int x, int y);
    void drawBeam(sf::Vector2f from, sf::Vector2f to);
    void drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color);
    void drawTileSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color);

    void renderPlayers(const std::map<int, PlayerState> &players, int me);
    bool updateAirState(int player_num);
    AnimationFrame pickAnimation(int player_num, const PlayerState &player, bool on_ground);
    void renderPlayerSprite(int player_num, const PlayerState &player, bool is_me);

    //===========================================================================
    // render_game.cpp: game screens, effects, particles, HUD
    //===========================================================================
    void renderGameScreen(GameState &state, Phase phase, float dt);
    void updateWorld(GameState &state, const std::map<int, PlayerState> &players, int me, float dt);
    void updateSounds(GameState &state, Phase phase, const std::map<int, PlayerState> &players, int me);
    void renderBanner();
    void renderCountdown(Phase phase, int countdown);
    void renderGameOver(GameState &state, const std::map<int, PlayerState> &players, int me);
    void renderSoloResults(const PlayerState &me);
    void renderVersusResults(const std::map<int, PlayerState> &players, int winner, int me);
    void renderRematchPrompt(GameState &state, bool solo);
    void renderConnectionLost();

    void addEffect(const CollisionEffect &effect, int me);
    void renderEffects();
    void emitParticles(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
                       float gravity, sf::Vector2f base_vel = {0, 0});
    void updateParticles(float dt);
    void renderParticles();
    void renderHUD(const std::map<int, PlayerState> &players, int me, bool cyberpsycho);
    void renderHUDLine(int player_num, const PlayerState &player, int me, float y);

    //===========================================================================
    // render_combat.cpp: CYBERPSYCHO drones, attacks and projectiles
    //===========================================================================
    void handleShots(GameState &state);
    sf::Vector2f shotPosition(const ShotTrail &shot) const;
    bool isLiveDrone(int x, int y, const std::set<std::pair<int, int>> &destroyed) const;
    void renderShots(const std::set<std::pair<int, int>> &destroyed);
    void renderDrone(int x, int y, float time);
    void drawDroneSprite(sf::Vector2f center, float width, int frame);

    //===========================================================================
    // render_titans.cpp: titans charging along the floor
    //===========================================================================
    void renderTitans(const std::vector<TitanState> &titans);
    void drawTitan(const TitanState &titan, float time);
    void renderTitanWarnings(const std::vector<TitanState> &titans);
    void updateTitanStomps(const std::vector<TitanState> &titans);

public:
    Renderer();

    bool initialize();
    void render(GameState &state, bool connected);
    void renderMenu(const MenuState &menu);
    void setLobbyInfo(const std::string &info) { lobby_info = info; }
    sf::RenderWindow &getWindow() { return window; }
    Audio &getAudio() { return audio; }
};

#endif // RENDER_HPP
