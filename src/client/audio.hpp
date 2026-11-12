#ifndef AUDIO_HPP
    #define AUDIO_HPP

#include <SFML/Audio.hpp>
#include <map>
#include <optional>
#include <vector>

// Music, sound effects synthesized at start-up, and the runners' recorded lines
class Audio {
public:
    // Each one is synthesized in Audio::synthesize(); the most heard are then replaced by the samples in
    // assets/sfx (loadSamples), and the recorded lines are loaded in loadLines()
    enum class Sfx {
        Coin, Zap, Beep, Go, GameOver, MenuMove, MenuConfirm,       // the run and the menus
        Gun, Minigun, Rocket, Slash, Hack, Thunder, Explosion,      // CYBERPSYCHO attacks and drones
        Stomp, Roar, Steam,                                         // titans and the Attack Titan
        Shield, ShieldBreak, Sandevistan, Berserk,                  // power-ups
        Siren, Jackpot, Pinata,                                     // upgrades
        FireWave, Nuke, Breach, Charge, UltimateReady,              // ultimates
        StyleUp,                                                    // Dante's style rank going up
        Blink,                                                      // Wraith's teleport
        JohnnyGun, JohnnyUlt, JinxUlt, DanteUlt,                    // recorded: assets/voices
        Kick,                                                       // DirtyBastrd's boot
        SawSnap, SawZap, SawClank, SawEngine,                       // his chainsaw
        DirtyBastrdEatThis,                                         // recorded: his lines
        DirtyBastrdHit, DirtyBastrdRareHit,
    };

    Audio();

    void playMusic(bool in_game);
    // Johnny's greeting, played once each time the main menu opens
    void greet();
    // Higher than 1, higher pitched (and faster). A recorded sound plays the take asked for, or any, at a share
    // of the effects' volume.
    void play(Sfx sfx, float pitch = 1, int take = -1, float volume = 1);
    // How many takes of a recorded sound were loaded
    int takes(Sfx sfx) const;
    // A sound looping for one owner (DirtyBastrd's saw: a player number) until stopped, at a share of the
    // effects' volume. Starting it again while it loops changes nothing.
    void startLoop(int owner, Sfx sfx, float volume);
    void setLoopPitch(int owner, float pitch);
    void stopLoop(int owner);
    void stopLoops();
    // Thrust rumble, looping while the jetpack is on
    void setJetpack(bool on);
    // Percentages, from the options
    void setVolumes(int music, int sfx);
    // Every frame: everything else fades under the recorded lines while they play
    void update();

private:
    static constexpr size_t VOICES = 12;
    // The voice lines have voices of their own: a row of coins or a payout's eddies, a dozen sounds in half a
    // second, would otherwise cut a line off before it's said
    static constexpr size_t LINE_VOICES = 4;

    sf::Music greeting;     // Johnny's line, once per visit to the menu
    sf::Music menu_music;   // loops out of the runs
    sf::Music game_music;   // loops during the runs

    std::map<Sfx, sf::SoundBuffer> buffers;
    // A recorded sound has a few takes: play() picks one at random
    std::map<Sfx, std::vector<sf::SoundBuffer>> lines;
    std::vector<sf::Sound> voices;     // played round-robin so effects can overlap
    std::vector<float> voice_shares;   // the share of the effects' volume each plays at
    size_t next_voice = 0;
    std::vector<sf::Sound> line_voices;
    std::vector<float> line_shares;
    size_t next_line_voice = 0;
    // A sound started again within this is skipped: twenty coins in one frame ring as one. At another
    // pitch it is another sound: the coins of a payout, each a little higher, all ring.
    static constexpr float REPEAT_GAP = 0.04f;
    std::map<std::pair<Sfx, float>, sf::Clock> last_played;

    struct Loop {
        sf::Sound sound;
        float volume;   // share of the effects' volume
    };
    std::map<int, Loop> loops;
    float sfx_volume = 70;
    float music_volume = 50;

    // While a voice line plays (his, Johnny's, Jinx's, Dante's), the effects, the saw, the jetpack and the
    // music drop to DUCK_LEVEL so the line cuts through a run's explosions and coins: down in DUCK_DOWN_TIME,
    // back up in DUCK_UP_TIME once no line has played for DUCK_HOLD_TIME, so lines back to back don't pump
    static constexpr float DUCK_LEVEL = 0.5f;
    static constexpr float DUCK_DOWN_TIME = 0.05f;
    static constexpr float DUCK_UP_TIME = 0.3f;
    static constexpr float DUCK_HOLD_TIME = 0.25f;
    float duck = 1;             // what everything but the lines plays at, from DUCK_LEVEL to 1
    float since_line = 1e9f;    // seconds since a line last played
    sf::Clock duck_clock;       // time between two updates

    sf::SoundBuffer jet_buffer;
    std::optional<sf::Sound> jet;

    void loadMusic();
    void loadLines();
    void synthesize();
    void loadSamples();
    void applyVolumes();
    // Spoken: plays on the line voices and ducks the rest. Johnny's gun is recorded but a gunshot, an effect.
    bool isVoice(Sfx sfx) const { return lines.count(sfx) > 0 && sfx != Sfx::JohnnyGun; }
    const sf::SoundBuffer *bufferOf(Sfx sfx, int take) const;
};

#endif
