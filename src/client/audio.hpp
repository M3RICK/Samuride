#ifndef AUDIO_HPP
    #define AUDIO_HPP

#include <SFML/Audio.hpp>
#include <map>
#include <optional>
#include <vector>

// Music, plus sound effects synthesized at start-up: no audio files to ship
class Audio {
public:
    enum class Sfx { Coin, Zap, Beep, Go, GameOver, MenuMove, MenuConfirm, Gun, Rocket, Slash, Explosion, Stomp, Roar, Steam,
                     Shield, ShieldBreak, Sandevistan, Hack, Berserk, FireWave, Nuke, Breach, Thunder, Minigun };

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
    std::vector<sf::Sound> voices;     // played round-robin so effects can overlap
    size_t next_voice = 0;

    sf::SoundBuffer jet_buffer;
    std::optional<sf::Sound> jet;

    void loadMusic();
    void synthesize();
};

#endif
