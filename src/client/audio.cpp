/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "audio.hpp"
#include "../common/debug.hpp"
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

    for (size_t i = 0; i < VOICES; i++)
        voices.emplace_back(buffers.at(Sfx::Coin));

    jet.emplace(jet_buffer);
    jet->setLooping(true);
    setVolumes(50, 70);
}

void Audio::loadMusic()
{
    if (menu_music.openFromFile("assets/music/waiting_screen.wav"))
        menu_music.setLooping(true);

    if (game_music.openFromFile("assets/music/ingame.wav"))
        game_music.setLooping(true);
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
    buffers[Sfx::Rocket] = toBuffer(mix(burst(0.3f, 0.3f, 0.3f), tone(Wave::Saw, 220, 90, 0.3f, 0.15f)));
    buffers[Sfx::Slash] = toBuffer(burst(0.15f, 0.25f, 0.9f));
    buffers[Sfx::Explosion] = toBuffer(mix(burst(0.6f, 0.45f, 0.08f), tone(Wave::Saw, 120, 40, 0.5f, 0.2f)));

    // No envelope: it loops
    jet_buffer = toBuffer(noise(1.0f, 0.5f, 0.05f));
}

void Audio::playMusic(bool in_game)
{
    sf::Music &wanted = in_game ? game_music : menu_music;
    sf::Music &other = in_game ? menu_music : game_music;

    if (wanted.getStatus() != sf::SoundSource::Status::Playing) {
        other.stop();
        wanted.play();
    }
}

void Audio::play(Sfx sfx)
{
    sf::Sound &voice = voices[next_voice];

    next_voice = (next_voice + 1) % VOICES;
    voice.setBuffer(buffers.at(sfx));
    voice.play();
}

// The in-game track and the jetpack sit a little under the rest of their group
void Audio::setVolumes(int music, int sfx)
{
    menu_music.setVolume(music);
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
