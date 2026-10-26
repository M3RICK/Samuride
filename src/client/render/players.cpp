// The players: animated runners with their jetpack exhaust, and the Attack Titan form

#include "players.hpp"
#include "canvas.hpp"
#include "sprites.hpp"
#include "particles.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include <cmath>
#include <cstdlib>

using namespace Palette;

Players::Players(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio, Particles &particles)
    : canvas(canvas), sprites(sprites), run(run), audio(audio), particles(particles) {
}

void Players::draw(const std::map<int, PlayerState> &players, int me, bool live)
{
    if (live)
        emitExhaust(players);

    // Others first so you are always drawn on top
    for (const auto &[player_num, player] : players) {
        if (player_num != me)
            drawPlayer(player_num, player, false);
    }

    auto mine = players.find(me);
    if (mine != players.end())
        drawPlayer(me, mine->second, true);
}

// Exhaust stays in the world, so it trails behind as the player runs
void Players::emitExhaust(const std::map<int, PlayerState> &players)
{
    for (const auto &[player_num, player] : players) {
        const Character &character = sprites.character(run.characterOf(player_num));
        sf::Vector2f nozzle = run.display_positions[player_num] + sf::Vector2f(0.25f, 1.55f);
        bool flying = player.form == Form::Normal || player.form == Form::Emerging;

        if (flying && player.jet_active) {
            particles.emit(nozzle, 2, character.exhaust, 1.5f, 0.35f, 0, {0, 8});
            particles.emit(nozzle, 1, NEON_ORANGE, 2.5f, 0.25f, 0, {0, 6});
        }
    }
}

// Returns whether the player is on the ground; leaving it starts the take-off clock
bool Players::updateAirState(int player_num)
{
    PlayerAnim &state = run.player_anims[player_num];
    bool on_ground = run.display_positions[player_num].y >= run.floor() - Run::PLAYER_HEIGHT - 0.01f;

    if (on_ground) {
        state.on_ground = true;
    } else if (state.on_ground) {
        state.on_ground = false;
        state.air_clock.restart();
    }
    return on_ground;
}

// Animation follows what the player does: run, take off, rise, hover at the ceiling, fall, attack, die
Players::AnimationFrame Players::pickAnimation(int player_num, const PlayerState &player, bool on_ground)
{
    const Character &character = sprites.character(run.characterOf(player_num));
    auto death = run.death_clocks.find(player_num);
    auto attack = run.attack_clocks.find(player_num);
    float air_time = run.player_anims[player_num].air_clock.getElapsedTime().asSeconds();
    float time = canvas.time();

    if (death != run.death_clocks.end() && character.death.frames > 0)
        return {&character.death, death->second.getElapsedTime().asSeconds()};
    const Animation &strike = run.strikeOf(player_num, character);
    if (attack != run.attack_clocks.end() && attack->second.getElapsedTime().asSeconds() < strike.duration())
        return {&strike, attack->second.getElapsedTime().asSeconds()};
    if (on_ground)
        return {&character.run, run.run_phase};
    if (air_time < character.takeoff.duration())
        return {&character.takeoff, air_time};
    if (!player.jet_active)
        return {&character.fall, time};

    bool at_ceiling = run.display_positions[player_num].y <= 0.01f;
    return {at_ceiling ? &character.hover : &character.rise, time};
}

void Players::drawPlayer(int player_num, const PlayerState &player, bool is_me)
{
    updateForm(player_num, player, is_me);
    updateSprint(player_num, player);
    if (player.form == Form::Shifting || player.form == Form::Titan) {
        drawShift(player_num, player, is_me);
        return;
    }
    if (player.form == Form::Emerging)
        drawSteamPuff(player_num);

    const Character &character = sprites.character(run.characterOf(player_num));
    AnimationFrame animation = pickAnimation(player_num, player, updateAirState(player_num));
    int frame = animation.animation->frameAt(animation.time);

    if (frame < 0 || character.sheet.getSize().x == 0)
        return;

    // Electrocution: shaking and flashing, for good if there is no death animation
    auto death = run.death_clocks.find(player_num);
    bool shocked = death != run.death_clocks.end() &&
                   (character.death.frames == 0 || death->second.getElapsedTime().asSeconds() < 0.4f);
    float tile_size = run.view.tile_size;
    sf::Vector2f feet = run.toScreen(run.display_positions[player_num] + sf::Vector2f(0.5f, Run::PLAYER_HEIGHT));
    float scale = tile_size * Run::PLAYER_HEIGHT * 1.05f / character.body;

    if (shocked)
        feet += {randf(-6, 6), randf(-6, 6)};
    // Pow-Pow's kick: pushed back a few pixels right after each bullet
    if (weaponOf(run.characterOf(player_num)) == Weapon::Minigun &&
        run.player_anims[player_num].shot_clock.getElapsedTime().asSeconds() < 0.05f)
        feet.x -= tile_size * 0.1f;

    sf::Sprite sprite(character.sheet);
    sprite.setTextureRect(character.frameRect(*animation.animation, frame));
    sprite.setOrigin({character.frame_size.x / 2.f, character.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(feet);

    if (player.sprinting)
        drawAfterimages(sprite, tile_size);
    if (player.berserk)
        canvas.halo(feet - sf::Vector2f(0, tile_size), tile_size * (0.9f + 0.15f * std::sin(canvas.time() * 16)),
                    withAlpha(BERSERK_COLOR, 70));
    if (player.shielded)
        drawShieldBubble(feet - sf::Vector2f(0, tile_size * Run::PLAYER_HEIGHT / 2), tile_size);
    if (shocked)
        sprite.setColor(std::rand() % 2 ? NEON_CYAN : sf::Color::White);
    else if (player.grace && std::fmod(canvas.time(), 0.16f) < 0.08f)
        sprite.setColor(sf::Color(255, 255, 255, 60));
    else if (!is_me)
        sprite.setColor(sf::Color(255, 255, 255, 215));
    canvas.draw(sprite);

    if (shocked && std::rand() % 2)
        canvas.draw(sprite, sf::BlendAdd);

    sf::Vector2f tag_pos = feet - sf::Vector2f(0, tile_size * (Run::PLAYER_HEIGHT + 0.25f));
    canvas.text(is_me ? "YOU" : "CHOOM", 22, is_me ? NEON_YELLOW : NEON_CYAN, tag_pos);

    // Lucy's RAM, over her head where it can be read without looking away from the run: thin red bars,
    // like the RAM units of Cyberpunk 2077, lit while loaded, hollow once spent
    if (run.cyberpsycho && weaponOf(run.characterOf(player_num)) == Weapon::Hack) {
        for (int i = 0; i < RAM_MAX; i++) {
            bool loaded = i < player.ram;
            sf::RectangleShape unit({9, 26});
            unit.setOrigin({4.5f, 13});
            unit.setPosition(tag_pos + sf::Vector2f((i - (RAM_MAX - 1) / 2.f) * 15, -40));
            unit.setFillColor(loaded ? NEON_RED : sf::Color(0, 0, 0, 120));
            unit.setOutlineColor(withAlpha(NEON_RED, loaded ? 255 : 140));
            unit.setOutlineThickness(2);
            canvas.draw(unit);
            if (loaded)
                canvas.draw(unit, sf::BlendAdd);
        }
    }

    // Mikasa's thunder spears left, over her head
    for (int i = 0; i < player.spears; i++) {
        sf::Vector2f tip = tag_pos + sf::Vector2f((i - (player.spears - 1) / 2.f) * 22, -62);
        sf::RectangleShape shaft({5, 30});
        shaft.setPosition(tip + sf::Vector2f(-2.5f, 12));
        shaft.setFillColor(NEON_CYAN);
        canvas.draw(shaft);
        sf::ConvexShape head(3);
        head.setPoint(0, tip);
        head.setPoint(1, tip + sf::Vector2f(-7, 14));
        head.setPoint(2, tip + sf::Vector2f(7, 14));
        head.setFillColor(sf::Color::White);
        canvas.draw(head);
    }
}

//=============================================================================
// Shield & Sandevistan
//=============================================================================

// Starts the Sandevistan and Berserk clocks, which drain your gauges in the HUD
void Players::updateSprint(int player_num, const PlayerState &player)
{
    PlayerAnim &state = run.player_anims[player_num];

    if (player.sprinting && !state.sprinting)
        state.sprint_clock.restart();
    state.sprinting = player.sprinting;
    if (player.berserk && !state.berserk)
        state.berserk_clock.restart();
    state.berserk = player.berserk;
}

// Sandevistan: ghosts of the runner trailing behind, alternating magenta and white
void Players::drawAfterimages(const sf::Sprite &sprite, float tile_size)
{
    sf::Sprite ghost = sprite;

    for (int i = 4; i >= 1; i--) {
        ghost.setPosition(sprite.getPosition() - sf::Vector2f(tile_size * 0.45f * i, 0));
        ghost.setColor(withAlpha(i % 2 ? SANDEVISTAN_COLOR : sf::Color::White, 230 - 40 * i));
        canvas.draw(ghost, sf::BlendAdd);
    }
}

// A hexagon plate behind the body, flickering a little
void Players::drawShieldBubble(sf::Vector2f center, float tile_size)
{
    float time = canvas.time();
    float radius = tile_size * (1.05f + 0.04f * std::sin(time * 9));

    canvas.halo(center, radius, withAlpha(SHIELD_COLOR, 35));
    canvas.hexagon(center, radius, withAlpha(SHIELD_COLOR, std::rand() % 10 ? 200 : 110), time * 25);
}

//=============================================================================
// Attack Titan
//=============================================================================

// Notices form changes: plays their sound, restarts the form clock, remembers where the titan stood
void Players::updateForm(int player_num, const PlayerState &player, bool is_me)
{
    PlayerAnim &state = run.player_anims[player_num];

    if (player.form == state.form)
        return;

    state.form = player.form;
    state.form_clock.restart();

    if (player.form == Form::Shifting) {
        audio.play(Audio::Sfx::Roar);
    } else if (player.form == Form::Titan) {
        audio.play(Audio::Sfx::Stomp);
        run.shakeAtLeast(is_me ? 0.4f : 0.2f);
    } else if (player.form == Form::Emerging) {
        audio.play(Audio::Sfx::Steam);
        state.smoke_x = run.display_positions[player_num].x + TITAN_FORM_WIDTH / 2;
    }
}

// Smoke column, then the titan rising from it, then the titan running; feet on the floor
void Players::drawShift(int player_num, const PlayerState &player, bool is_me)
{
    const PlayerAnim &state = run.player_anims[player_num];
    float time = state.form_clock.getElapsedTime().asSeconds();
    float center_x = run.display_positions[player_num].x + TITAN_FORM_WIDTH / 2;
    uint8_t alpha = is_me ? 255 : 215;

    if (player.form == Form::Shifting) {
        const Animation &smoke = sprites.shift.smoke;
        const Animation &anim = time < smoke.duration() ? smoke : sprites.shift.emerge;
        float anim_time = &anim == &smoke ? time : time - smoke.duration();
        drawShiftFrame(sprites.shift, anim, anim.frameAt(anim_time), center_x, alpha);
        return;
    }

    // Heavy steps; the titan sheet runs to the left, flipped to run with you
    const Character &titan = sprites.eren_titan;
    if (static_cast<int>(run.run_phase * 2.2f) != static_cast<int>((run.run_phase - 1 / 60.f) * 2.2f))
        run.shakeAtLeast(is_me ? 0.08f : 0.04f);
    drawShiftFrame(titan, titan.run, titan.run.frameAt(run.run_phase), center_x, alpha, true);
}

// Bursting out: the smoke the titan left shrinks and fades where it stood
void Players::drawSteamPuff(int player_num)
{
    const PlayerAnim &state = run.player_anims[player_num];
    float t = state.form_clock.getElapsedTime().asSeconds() / EMERGE_TIME;
    const Animation &smoke = sprites.shift.smoke;

    if (t >= 1)
        return;
    int frame = static_cast<int>((smoke.frames - 1) * (1 - t));
    drawShiftFrame(sprites.shift, smoke, frame, state.smoke_x, static_cast<uint8_t>(230 * (1 - t)));
}

// Feet on the floor, `sheet`'s standing height scaled to the titan form's
void Players::drawShiftFrame(const Character &sheet, const Animation &anim, int frame, float center_x, uint8_t alpha,
                             bool flip)
{
    if (frame < 0 || sheet.sheet.getSize().x == 0)
        return;

    float scale = run.view.tile_size * TITAN_FORM_HEIGHT / sheet.body;
    sf::Sprite sprite(sheet.sheet);

    sprite.setTextureRect(sheet.frameRect(anim, frame));
    sprite.setOrigin({sheet.frame_size.x / 2.f, sheet.feet});
    sprite.setScale({flip ? -scale : scale, scale});
    sprite.setPosition(run.toScreen({center_x, run.floor()}));
    sprite.setColor(withAlpha(sf::Color::White, alpha));
    canvas.draw(sprite);
}
