#ifndef AUDIO_HPP
    #define AUDIO_HPP

#include <SFML/Audio.hpp>
#include <map>
#include <optional>
#include <vector>

// Music, sound effects synthesized at start-up, and the runners' recorded lines
class Audio {
public:
    // Each one is synthesized in Audio::synthesize(), but the recorded ones loaded in loadLines()
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
    };

    Audio();

    void playMusic(bool in_game);
    // Johnny's greeting, played once each time the main menu opens
    void greet();
    void play(Sfx sfx);
    // Thrust rumble, looping while the jetpack is on
    void setJetpack(bool on);
    // Percentages, from the options
    void setVolumes(int music, int sfx);

private:
    static constexpr size_t VOICES = 12;

    sf::Music greeting;     // Johnny's line, once per visit to the menu
    sf::Music menu_music;   // loops out of the runs
    sf::Music game_music;   // loops during the runs

    std::map<Sfx, sf::SoundBuffer> buffers;
    // A recorded sound has a few takes: play() picks one at random
    std::map<Sfx, std::vector<sf::SoundBuffer>> lines;
    std::vector<sf::Sound> voices;     // played round-robin so effects can overlap
    size_t next_voice = 0;
    // A sound started again within this is skipped: twenty coins in one frame ring as one
    static constexpr float REPEAT_GAP = 0.04f;
    std::map<Sfx, sf::Clock> last_played;

    sf::SoundBuffer jet_buffer;
    std::optional<sf::Sound> jet;

    void loadMusic();
    void loadLines();
    void synthesize();
};

#endif
