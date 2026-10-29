// Music, and the sound effects synthesized from oscillators and noise

#include "audio.hpp"
#include "../common/debug.hpp"
#include "../common/ultimates.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

//=============================================================================
// Synthesis: samples in -1..1, built from simple oscillators and noise
//=============================================================================

namespace {

constexpr unsigned RATE = 44100;
constexpr float TAU = 6.2831853f;

using Samples = std::vector<float>;

enum class Wave { Sine, Square, Saw };

// One sample of a waveform, `phase` counted in cycles
float oscillator(Wave wave, float phase)
{
    float p = phase - std::floor(phase);

    switch (wave) {
        case Wave::Sine:
            return std::sin(TAU * p);
        case Wave::Square:
            return p < 0.5f ? 1.f : -1.f;
        case Wave::Saw:
            return 2 * p - 1;
    }
    return 0;
}

// 5 ms attack, then a straight fade out: no clicks at either end
float envelope(size_t index, size_t count)
{
    float attack = std::min(1.f, index / (RATE * 0.005f));

    return attack * (1 - static_cast<float>(index) / count);
}

// A note whose pitch slides from `from` to `to` Hz
Samples tone(Wave wave, float from, float to, float seconds, float volume)
{
    size_t count = seconds * RATE;
    Samples out(count);
    float phase = 0;

    for (size_t i = 0; i < count; i++) {
        phase += (from + (to - from) * i / count) / RATE;
        out[i] = oscillator(wave, phase) * envelope(i, count) * volume;
    }
    return out;
}

// White noise through a one-pole low-pass: the lower `smoothing`, the darker the rumble
Samples noise(float seconds, float volume, float smoothing)
{
    std::mt19937 rng(7);    // fixed seed: the same sounds every launch
    std::uniform_real_distribution<float> white(-1, 1);
    size_t count = seconds * RATE;
    Samples out(count);
    float value = 0;

    for (size_t i = 0; i < count; i++) {
        value += (white(rng) - value) * smoothing;
        out[i] = value * volume;
    }
    return out;
}

Samples concat(Samples first, const Samples &second)
{
    first.insert(first.end(), second.begin(), second.end());
    return first;
}

// Electric buzz: a low saw over crackling noise chopped 40 times a second
Samples zap()
{
    Samples buzz = tone(Wave::Saw, 95, 70, 0.45f, 0.2f);
    Samples crackle = noise(0.45f, 0.3f, 0.6f);

    for (size_t i = 0; i < buzz.size(); i++) {
        bool open = static_cast<int>(i * 40 / RATE) % 2 == 0;
        buzz[i] += crackle[i] * envelope(i, crackle.size()) * (open ? 1 : 0.3f);
    }
    return buzz;
}

// Noise whose loudness follows the envelope, for whooshes and blasts
Samples burst(float seconds, float volume, float smoothing)
{
    Samples out = noise(seconds, volume, smoothing);

    for (size_t i = 0; i < out.size(); i++)
        out[i] *= envelope(i, out.size());
    return out;
}

Samples mix(Samples first, const Samples &second)
{
    first.resize(std::max(first.size(), second.size()), 0);
    for (size_t i = 0; i < second.size(); i++)
        first[i] += second[i];
    return first;
}

sf::SoundBuffer toBuffer(const Samples &samples)
{
    std::vector<std::int16_t> pcm(samples.size());
    sf::SoundBuffer buffer;

    for (size_t i = 0; i < samples.size(); i++)
        pcm[i] = std::clamp(samples[i], -1.f, 1.f) * 32767;

    if (!buffer.loadFromSamples(pcm.data(), pcm.size(), 1, RATE, {sf::SoundChannel::Mono}))
        DEBUG_LOG("Failed to create a sound buffer");
    return buffer;
}

}

//=============================================================================
// Audio
//=============================================================================

Audio::Audio()
{
    loadMusic();
    synthesize();
    loadLines();

    for (size_t i = 0; i < VOICES; i++)
        voices.emplace_back(buffers.at(Sfx::Coin));

    jet.emplace(jet_buffer);
    jet->setLooping(true);
    setVolumes(50, 70);
}

void Audio::loadMusic()
{
    // Johnny's greeting: once per visit to the menu, see greet()
    if (!greeting.openFromFile("assets/music/waiting_screen.wav"))
        DEBUG_LOG("Failed to load Johnny's greeting");

    if (menu_music.openFromFile("assets/music/menu.wav"))
        menu_music.setLooping(true);
    if (game_music.openFromFile("assets/music/run.mp3"))
        game_music.setLooping(true);
}

void Audio::loadLines()
{
    const std::map<Sfx, std::vector<const char *>> files = {
        {Sfx::JohnnyGun, {"johny_gun_low", "johny_gun_loud"}},
        {Sfx::JohnnyUlt, {"johny_ult"}},
        {Sfx::JinxUlt, {"jinx_ult_byebye", "jinx_ult_catch", "jinx_ult_huh", "jinx_ult_seeya",
                        "jinx_ult_watchthis", "jinx_ult_wo"}},
        {Sfx::DanteUlt, {"dante_ult_jackpot"}},
    };

    for (const auto &[sfx, names] : files)
        for (const char *name : names) {
            sf::SoundBuffer buffer;
            if (buffer.loadFromFile(std::string("assets/voices/") + name + ".ogg"))
                lines[sfx].push_back(std::move(buffer));
            else
                DEBUG_LOG("Failed to load a voice line");
        }
}

// Retro arcade palette: square-wave blips, saw sweeps and filtered noise
void Audio::synthesize()
{
    buffers[Sfx::Coin] = toBuffer(concat(tone(Wave::Square, 988, 988, 0.07f, 0.25f),
                                         tone(Wave::Square, 1319, 1319, 0.2f, 0.25f)));
    buffers[Sfx::Zap] = toBuffer(zap());
    buffers[Sfx::Beep] = toBuffer(tone(Wave::Square, 880, 880, 0.12f, 0.2f));
    buffers[Sfx::Go] = toBuffer(tone(Wave::Square, 1320, 1760, 0.35f, 0.22f));
    buffers[Sfx::GameOver] = toBuffer(tone(Wave::Saw, 600, 120, 0.8f, 0.3f));
    buffers[Sfx::MenuMove] = toBuffer(tone(Wave::Square, 1500, 1500, 0.03f, 0.12f));
    buffers[Sfx::MenuConfirm] = toBuffer(tone(Wave::Square, 700, 1400, 0.12f, 0.18f));

    // CYBERPSYCHO weapons and drones
    buffers[Sfx::Gun] = toBuffer(tone(Wave::Square, 1200, 300, 0.08f, 0.15f));
    // Ultimate ready: a bright rising arpeggio
    buffers[Sfx::UltimateReady] = toBuffer(concat(concat(tone(Wave::Square, 660, 660, 0.08f, 0.18f),
                                                         tone(Wave::Square, 880, 880, 0.08f, 0.18f)),
                                                  tone(Wave::Square, 1320, 1320, 0.2f, 0.18f)));
    // Dante's style rank going up: a short distorted power chord, like DMC's rank stingers
    buffers[Sfx::StyleUp] = toBuffer(concat(mix(tone(Wave::Saw, 165, 165, 0.07f, 0.16f), tone(Wave::Saw, 247, 247, 0.07f, 0.12f)),
                                            mix(tone(Wave::Saw, 220, 220, 0.18f, 0.16f), tone(Wave::Saw, 330, 330, 0.18f, 0.12f))));
    // Pow-Pow: short and sharp, it fires 8 times a second
    buffers[Sfx::Minigun] = toBuffer(tone(Wave::Square, 900, 450, 0.04f, 0.1f));
    buffers[Sfx::Rocket] = toBuffer(mix(burst(0.3f, 0.3f, 0.3f), tone(Wave::Saw, 220, 90, 0.3f, 0.15f)));
    buffers[Sfx::Slash] = toBuffer(burst(0.15f, 0.25f, 0.9f));
    // Berserk: a low growl revving up
    buffers[Sfx::Berserk] = toBuffer(mix(tone(Wave::Saw, 90, 260, 0.45f, 0.3f), tone(Wave::Square, 180, 520, 0.45f, 0.12f)));
    // Johnny's fire wave: a roaring blast
    buffers[Sfx::FireWave] = toBuffer(mix(burst(1.0f, 0.5f, 0.15f), tone(Wave::Saw, 160, 50, 0.9f, 0.25f)));
    // Fishbones charging: a whine climbing for as long as the charge lasts
    buffers[Sfx::Charge] = toBuffer(mix(tone(Wave::Saw, 180, 1400, FISHBONES_CHARGE_TIME, 0.18f),
                                        tone(Wave::Square, 90, 700, FISHBONES_CHARGE_TIME, 0.08f)));
    // David's nuke: a deep, long blast
    buffers[Sfx::Nuke] = toBuffer(mix(burst(1.4f, 0.6f, 0.05f), tone(Wave::Sine, 90, 25, 1.4f, 0.5f)));
    // Mikasa's thunder spears: a sharp crack, then the rumble
    buffers[Sfx::Thunder] = toBuffer(concat(burst(0.08f, 0.6f, 0.95f), mix(burst(0.6f, 0.35f, 0.1f),
                                                                             tone(Wave::Saw, 140, 45, 0.6f, 0.2f))));
    // Lucy's breach: a long stuttering sweep of data
    buffers[Sfx::Breach] = toBuffer(concat(tone(Wave::Square, 300, 2400, 0.25f, 0.15f),
                                           mix(tone(Wave::Square, 2400, 200, 0.4f, 0.12f), burst(0.4f, 0.2f, 0.9f))));
    // Lucy's hack: a glitchy data chirp
    buffers[Sfx::Hack] = toBuffer(concat(tone(Wave::Square, 2400, 800, 0.06f, 0.12f),
                                         tone(Wave::Square, 600, 1800, 0.1f, 0.12f)));
    // Attack Titan: a growl rising out of the smoke, then the steam burst when it ends
    buffers[Sfx::Roar] = toBuffer(mix(tone(Wave::Saw, 80, 140, 1.2f, 0.3f), burst(1.2f, 0.2f, 0.08f)));
    buffers[Sfx::Steam] = toBuffer(burst(0.7f, 0.4f, 0.5f));
    buffers[Sfx::Stomp] = toBuffer(mix(tone(Wave::Sine, 70, 40, 0.35f, 0.5f), burst(0.2f, 0.25f, 0.05f)));
    buffers[Sfx::Explosion] = toBuffer(mix(burst(0.6f, 0.45f, 0.08f), tone(Wave::Saw, 120, 40, 0.5f, 0.2f)));
    // Power-ups: a rising chime, the plate shattering, the Sandevistan's whoosh
    buffers[Sfx::Shield] = toBuffer(concat(tone(Wave::Sine, 520, 1040, 0.15f, 0.3f),
                                           tone(Wave::Sine, 1040, 1560, 0.15f, 0.25f)));
    buffers[Sfx::ShieldBreak] = toBuffer(mix(burst(0.35f, 0.35f, 0.9f), tone(Wave::Square, 1800, 400, 0.25f, 0.15f)));
    // Corvo's Blink: an airy whoosh sucked in, over a deep thump
    buffers[Sfx::Blink] = toBuffer(mix(mix(burst(0.18f, 0.35f, 0.4f), tone(Wave::Sine, 1600, 200, 0.18f, 0.2f)),
                                       tone(Wave::Sine, 140, 35, 0.22f, 0.5f)));
    buffers[Sfx::Sandevistan] = toBuffer(mix(tone(Wave::Saw, 200, 1400, 0.4f, 0.2f), burst(0.4f, 0.2f, 0.6f)));

    // Trauma Team: a two-tone siren; Lucky Chip: a slot machine paying out; Drone Pinata: a pop, then a chime
    buffers[Sfx::Siren] = toBuffer(concat(concat(tone(Wave::Square, 960, 960, 0.18f, 0.16f), tone(Wave::Square, 720, 720, 0.18f, 0.16f)),
                                          concat(tone(Wave::Square, 960, 960, 0.18f, 0.16f), tone(Wave::Square, 720, 720, 0.25f, 0.16f))));
    buffers[Sfx::Jackpot] = toBuffer(concat(concat(concat(tone(Wave::Square, 1047, 1047, 0.06f, 0.2f), tone(Wave::Square, 1319, 1319, 0.06f, 0.2f)),
                                                   concat(tone(Wave::Square, 1568, 1568, 0.06f, 0.2f), tone(Wave::Square, 2093, 2093, 0.06f, 0.2f))),
                                            mix(tone(Wave::Square, 2093, 2093, 0.3f, 0.15f), burst(0.3f, 0.1f, 0.95f))));
    buffers[Sfx::Pinata] = toBuffer(concat(burst(0.08f, 0.5f, 0.7f), tone(Wave::Sine, 1568, 2093, 0.25f, 0.25f)));

    // No envelope: it loops
    jet_buffer = toBuffer(noise(1.0f, 0.5f, 0.05f));
}

// One track loops in the runs, another out of them once Johnny's greeting is over
void Audio::playMusic(bool in_game)
{
    sf::Music &wanted = in_game ? game_music : menu_music;
    sf::Music &other = in_game ? menu_music : game_music;

    other.stop();
    if (in_game)
        greeting.stop();
    else if (greeting.getStatus() == sf::SoundSource::Status::Playing)
        return;
    if (wanted.getStatus() != sf::SoundSource::Status::Playing)
        wanted.play();
}

void Audio::greet()
{
    menu_music.stop();
    greeting.stop();
    greeting.play();
}

void Audio::play(Sfx sfx)
{
    auto last = last_played.find(sfx);
    if (last != last_played.end() && last->second.getElapsedTime().asSeconds() < REPEAT_GAP)
        return;
    last_played[sfx].restart();

    sf::Sound &voice = voices[next_voice];

    next_voice = (next_voice + 1) % VOICES;
    static std::mt19937 rng(std::random_device{}());
    auto takes = lines.find(sfx);
    if (takes != lines.end())
        voice.setBuffer(takes->second[std::uniform_int_distribution<size_t>(0, takes->second.size() - 1)(rng)]);
    else if (buffers.count(sfx))
        voice.setBuffer(buffers.at(sfx));
    else
        return;
    voice.play();
}

// The tracks and the jetpack sit a little under the rest of their group
void Audio::setVolumes(int music, int sfx)
{
    greeting.setVolume(music);
    menu_music.setVolume(music * 0.8f);
    game_music.setVolume(music * 0.8f);

    for (sf::Sound &voice : voices)
        voice.setVolume(sfx);
    jet->setVolume(sfx * 0.5f);
}

void Audio::setJetpack(bool on)
{
    bool playing = jet->getStatus() == sf::SoundSource::Status::Playing;

    if (on && !playing)
        jet->play();
    else if (!on && playing)
        jet->stop();
}
