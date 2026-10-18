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

struct MenuState;

class Renderer {
private:
    struct ActiveEffect {
        CollisionEffect effect;
        sf::Clock clock;
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

    sf::RenderWindow window;

    // Game assets
    sf::Texture waiting_screen_texture;
    sf::Texture coin_texture;
    sf::Texture electric_texture;
    sf::Font font;
    // The jetpack font has no punctuation: text that needs some falls back to this one
    sf::Font fallback_font;
    std::string missing_glyphs;

    // A district's look: one image cut into horizontal layers, each scrolling at its own speed.
    // A layer can also chain several pictures one after the other, to avoid looping too soon.
    struct Backdrop {
        struct Segment {
            int texture;        // 0 is the main texture, then extra_textures[n - 1]
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
        sf::Texture texture;
        std::vector<sf::Texture> extra_textures;
        // Calibration knobs: image height fractions landing on the top of the map and on the feet line
        float ceiling;
        float floor;
        sf::Color tint;
        std::vector<Layer> layers;
    };
    std::vector<Backdrop> backdrops;

    // District banner ("ENTERING ..."), shown when the scenery changes
    int shown_backdrop;
    std::string banner;
    std::string banner_tagline;
    sf::Clock banner_clock;

    // One per CHARACTER_DIRS entry, and who plays which in the current run
    std::vector<Character> characters;
    std::map<int, int> roster;

    // Audio
    sf::Music waiting_music;
    sf::Music background_music;

    // The run, rebuilt from the chunks the server streams
    Map world_map;
    std::string lobby_info;

    // Camera & world, in tiles
    float camera_x;
    float tile_size;
    std::map<int, sf::Vector2f> display_positions;

    std::vector<ActiveEffect> effects;
    std::vector<Particle> particles;
    struct PlayerAnim {
        bool on_ground = true;
        sf::Clock air_clock;
    };
    std::map<int, PlayerAnim> player_anims;
    std::map<int, sf::Clock> death_clocks;
    float shake;

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

    // Night City palette
    static constexpr sf::Color NEON_YELLOW{252, 238, 10};
    static constexpr sf::Color NEON_CYAN{0, 240, 255};
    static constexpr sf::Color NEON_RED{255, 0, 60};
    static constexpr sf::Color NEON_PINK{255, 40, 200};
    static constexpr sf::Color NEON_ORANGE{255, 140, 0};
    static constexpr sf::Color DIM_WHITE{200, 200, 200};

    //===========================================================================
    // Initialization Helpers
    //===========================================================================
    void createWindow();
    void loadAssets();
    void loadTexture(sf::Texture &texture, const std::string &path);
    void loadAudio();

    //===========================================================================
    // Screens
    //===========================================================================
    void beginFrame(bool in_game);
    void endFrame();
    void onPhaseChange(GameState &state, Phase phase);
    void renderPoster();
    void renderCharacterSelect(const MenuState &menu);
    void renderDistrictSelect(const MenuState &menu);
    void renderWaitingScreen();
    void renderGameScreen(GameState &state, Phase phase, float dt);
    void renderCountdown(Phase phase, int countdown);
    void renderGameOver(GameState &state, const std::map<int, PlayerState> &players, int me);
    void renderConnectionLost();

    //===========================================================================
    // World Rendering
    //===========================================================================
    sf::Vector2f toScreen(sf::Vector2f world) const;
    void smoothPositions(const std::map<int, PlayerState> &players, float dt);
    void updateCamera(int me);
    void renderBackdrop(const Backdrop &backdrop, float opacity);
    void renderDistrict(int district);
    void drawThumbnail(const Backdrop &backdrop, sf::FloatRect dest, float from, float to);
    void renderMapTiles(const std::set<std::pair<int, int>> &collected);
    void renderZapper(int x, int y);
    void drawBeam(sf::Vector2f from, sf::Vector2f to);
    void drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color);
    void drawTileSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color);
    const Character &characterOf(int player_num) const;
    void renderPlayers(const std::map<int, PlayerState> &players, int me);
    void renderPlayerSprite(int player_num, const PlayerState &player, bool is_me);

    //===========================================================================
    // Effects, Particles & HUD
    //===========================================================================
    void addEffect(const CollisionEffect &effect, int me);
    void renderEffects();
    void emitParticles(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
                       float gravity, sf::Vector2f base_vel = {0, 0});
    void updateParticles(float dt);
    void renderParticles();
    void renderHUD(const std::map<int, PlayerState> &players, int me);

    //===========================================================================
    // Helper Functions
    //===========================================================================
    bool blink() const;
    sf::Text makeText(const std::string &content, unsigned int size, sf::Color color,
                      sf::Vector2f pos, sf::Vector2f anchor = {0.5f, 0.5f});
    void drawGlitchText(const std::string &content, unsigned int size, sf::Color color, sf::Vector2f pos);
    std::string playerLabel(int player_num, int me) const;

public:
    Renderer();

    bool initialize();
    void render(GameState &state, bool connected);
    void renderMenu(const MenuState &menu);
    void setLobbyInfo(const std::string &info) { lobby_info = info; }
    sf::RenderWindow &getWindow() { return window; }
};

#endif // RENDER_HPP
