/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include <algorithm>
#include <cmath>

// The Attack Titan power-up: the orb, the smoke column, the titan, and bursting back out.
// Forms come from the server; how long a player has been in one is timed here.

// Orb with the titan's face, on an orange-ringed disc
void Renderer::renderShiftOrb(int x, int y, float time)
{
    sf::Vector2f center = toScreen({x + 0.5f, y + 0.5f + std::sin(time * 3 + x) * 0.1f});
    float pulse = 0.5f + 0.5f * std::sin(time * 6);

    drawHalo(center, tile_size * (0.7f + 0.15f * pulse), withAlpha(NEON_ORANGE, 70));
    drawBacking(center, tile_size * 0.45f, withAlpha(NEON_ORANGE, 230));
    drawImage(titan_face, center, tile_size * 0.7f, {0.5f, 0.5f}, sf::Color::White);
}

// Notices form changes: plays their sound, restarts the form clock, remembers where the titan stood
void Renderer::updateForm(int player_num, const PlayerState &player, bool is_me)
{
    PlayerAnim &state = player_anims[player_num];

    if (player.form == state.form)
        return;

    state.form = player.form;
    state.form_clock.restart();

    if (player.form == Form::Shifting) {
        audio.play(Audio::Sfx::Roar);
    } else if (player.form == Form::Titan) {
        audio.play(Audio::Sfx::Stomp);
        shake = std::max(shake, is_me ? 0.4f : 0.2f);
    } else if (player.form == Form::Emerging) {
        audio.play(Audio::Sfx::Steam);
        state.smoke_x = display_positions[player_num].x + TITAN_FORM_WIDTH / 2;
    }
}

// Smoke column, then the titan rising from it, then the titan running; feet on the floor
void Renderer::drawShift(int player_num, const PlayerState &player, bool is_me)
{
    const PlayerAnim &state = player_anims[player_num];
    float time = state.form_clock.getElapsedTime().asSeconds();
    const Animation *anim = &shift_sprite.run;

    if (player.form == Form::Shifting) {
        anim = time < shift_sprite.smoke.duration() ? &shift_sprite.smoke : &shift_sprite.emerge;
        time = anim == &shift_sprite.smoke ? time : time - shift_sprite.smoke.duration();
    } else {
        time = run_phase;
        // Heavy steps
        if (static_cast<int>(time * 2.2f) != static_cast<int>((time - 1 / 60.f) * 2.2f))
            shake = std::max(shake, is_me ? 0.08f : 0.04f);
    }

    float center_x = display_positions[player_num].x + TITAN_FORM_WIDTH / 2;
    drawShiftFrame(*anim, anim->frameAt(time), center_x, is_me ? 255 : 215);
}

// Bursting out: the smoke the titan left shrinks and fades where it stood
void Renderer::drawSteamPuff(int player_num)
{
    const PlayerAnim &state = player_anims[player_num];
    float t = state.form_clock.getElapsedTime().asSeconds() / EMERGE_TIME;
    const Animation &smoke = shift_sprite.smoke;

    if (t >= 1)
        return;
    int frame = static_cast<int>((smoke.frames - 1) * (1 - t));
    drawShiftFrame(smoke, frame, state.smoke_x, static_cast<uint8_t>(230 * (1 - t)));
}

void Renderer::drawShiftFrame(const Animation &anim, int frame, float center_x, uint8_t alpha)
{
    if (frame < 0 || shift_sprite.sheet.getSize().x == 0)
        return;

    float scale = tile_size * TITAN_FORM_HEIGHT / shift_sprite.body;
    sf::Sprite sprite(shift_sprite.sheet);

    sprite.setTextureRect(shift_sprite.frameRect(anim, frame));
    sprite.setOrigin({shift_sprite.frame_size.x / 2.f, shift_sprite.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(toScreen({center_x, static_cast<float>(world_map.getHeight())}));
    sprite.setColor(withAlpha(sf::Color::White, alpha));
    window.draw(sprite);
}

// Draining gauge while you are the titan
void Renderer::renderShiftGauge(const PlayerState &player, int me)
{
    if (player.form != Form::Titan)
        return;

    float left = 1 - player_anims[me].form_clock.getElapsedTime().asSeconds() / TITAN_TIME;
    sf::Vector2f pos(SCREEN_WIDTH / 2 - 250, 40);

    sf::RectangleShape back({500, 18});
    back.setPosition(pos);
    back.setFillColor(sf::Color(0, 0, 0, 160));
    back.setOutlineColor(NEON_ORANGE);
    back.setOutlineThickness(2);
    window.draw(back);

    sf::RectangleShape fill({500 * std::clamp(left, 0.0f, 1.0f), 18});
    fill.setPosition(pos);
    fill.setFillColor(NEON_ORANGE);
    window.draw(fill);

    window.draw(makeText("ATTACK TITAN", 26, NEON_ORANGE, pos + sf::Vector2f(250, 40)));
}
