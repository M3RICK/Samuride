// The players: animated runners with their jetpack exhaust, and the Attack Titan form

#include "players.hpp"
#include "canvas.hpp"
#include "sprites.hpp"
#include "particles.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include "../../common/upgrades.hpp"
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

// A death that ends on the floor, not one that vanishes (Jinx blowing up)
static bool fallsDead(const Character &character)
{
    return character.death.frames > 0 && character.death.mode == Animation::Mode::Once;
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
    // Down until the Trauma Team's medic is done: their death if it ends on the floor, held on its last frame
    float rescue = run.rescueShare(player_num);
    if (rescue >= 0 && rescue < Run::RESCUE_UP)
        return {fallsDead(character) ? &character.death : &character.fall, rescue * TRAUMA_RESCUE_TIME};
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
    // Down without a death that falls: their fall, laid on its back, head toward the medic on the left
    float rescue = run.rescueShare(player_num);
    bool down = rescue >= 0 && rescue < Run::RESCUE_UP;
    if (down && !fallsDead(character)) {
        sprite.setRotation(sf::degrees(-90));
        sprite.setPosition(feet + sf::Vector2f(tile_size, -tile_size * 0.45f));
    }

    if (player.sprinting)
        drawAfterimages(sprite, tile_size);
    if (player.berserk)
        canvas.halo(feet - sf::Vector2f(0, tile_size), tile_size * (0.9f + 0.15f * std::sin(canvas.time() * 16)),
                    withAlpha(BERSERK_COLOR, 70));
    if (player.shielded)
        drawShieldBubble(feet - sf::Vector2f(0, tile_size * Run::PLAYER_HEIGHT / 2), tile_size);
    if (shocked)
        sprite.setColor(std::rand() % 2 ? NEON_CYAN : sf::Color::White);
    else if (player.grace && !down && std::fmod(canvas.time(), 0.16f) < 0.08f)
        sprite.setColor(sf::Color(255, 255, 255, 60));
    else if (!is_me)
        sprite.setColor(sf::Color(255, 255, 255, 215));
    canvas.draw(sprite);

    if (shocked && std::rand() % 2)
        canvas.draw(sprite, sf::BlendAdd);

    sf::Vector2f tag_pos = feet - sf::Vector2f(0, tile_size * (down ? 1.2f : Run::PLAYER_HEIGHT + 0.25f));
    canvas.text(is_me ? "YOU" : "CHOOM", 22, is_me ? NEON_YELLOW : NEON_CYAN, tag_pos);

    // Lucy's RAM (or Wraith's Blinks, in cyan), over her head where it can be read without looking away
    // from the run: thin bars, like the RAM units of Cyberpunk 2077, lit while loaded, hollow once spent
    Weapon weapon = weaponOf(run.characterOf(player_num));
    if (run.cyberpsycho && (weapon == Weapon::Hack || weapon == Weapon::Blink)) {
        int units = weapon == Weapon::Hack ? RAM_MAX : BLINK_CHARGES;
        sf::Color color = weapon == Weapon::Hack ? NEON_RED : NEON_CYAN;
        for (int i = 0; i < units; i++) {
            bool loaded = i < player.ram;
            sf::RectangleShape unit({9, 26});
            unit.setOrigin({4.5f, 13});
            unit.setPosition(tag_pos + sf::Vector2f((i - (units - 1) / 2.f) * 15, -40));
            unit.setFillColor(loaded ? color : sf::Color(0, 0, 0, 120));
            unit.setOutlineColor(withAlpha(color, loaded ? 255 : 140));
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

// Where Mikasa sits on Eren's titan: tiles ahead of his centre, and over the floor
static constexpr float SHOULDER_X = -0.3f;
static constexpr float SHOULDER_Y = 3.4f;

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

// Smoke column, then the titan rising from it (or the runner's mount coming out of it), then running;
// feet on the floor
void Players::drawShift(int player_num, const PlayerState &player, bool is_me)
{
    const PlayerAnim &state = run.player_anims[player_num];
    float time = state.form_clock.getElapsedTime().asSeconds();
    float center_x = run.display_positions[player_num].x + TITAN_FORM_WIDTH / 2;
    uint8_t alpha = is_me ? 255 : 215;
    const Character &mount = sprites.mount(run.characterOf(player_num));
    bool titan = &mount == &sprites.eren_titan;

    if (player.form == Form::Shifting) {
        const Animation &smoke = sprites.shift.smoke;
        if (titan || time < smoke.duration()) {
            const Animation &anim = time < smoke.duration() ? smoke : sprites.shift.emerge;
            float anim_time = &anim == &smoke ? time : time - smoke.duration();
            drawShiftFrame(sprites.shift, anim, anim.frameAt(anim_time), center_x, alpha);
            return;
        }
        // The mount shows through the smoke as it clears
        float clear = std::clamp((time - smoke.duration()) / std::max(0.1f, SHIFT_TIME - smoke.duration()), 0.0f, 1.0f);
        drawShiftFrame(mount, mount.run, 0, center_x, static_cast<uint8_t>(alpha * clear), false, mount.height);
        drawShiftFrame(sprites.shift, smoke, smoke.frames - 1, center_x, static_cast<uint8_t>(alpha * (1 - clear)));
        return;
    }

    if (!titan) {
        drawShiftFrame(mount, mount.run, mount.run.frameAt(run.run_phase), center_x, alpha, false, mount.height);
        return;
    }
    // Heavy steps; the titan sheet runs to the left, flipped to run with you
    if (static_cast<int>(run.run_phase * 2.2f) != static_cast<int>((run.run_phase - 1 / 60.f) * 2.2f))
        run.shakeAtLeast(is_me ? 0.08f : 0.04f);
    int frame = mount.run.frameAt(run.run_phase);
    drawShiftFrame(mount, mount.run, frame, center_x, alpha, true);
    // Mikasa rides on his shoulder, crouched (her death's fourth frame), rocked by his steps
    const Character &runner = sprites.character(run.characterOf(player_num));
    if (runner.name == "MIKASA" && runner.death.frames > 3)
        drawShiftFrame(runner, runner.death, 3, center_x + SHOULDER_X, alpha, false, 2.0f,
                       SHOULDER_Y + 0.08f * std::sin(frame * 3.14159f / 3));
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

// Feet on the floor (or `lift` tiles over it), `sheet`'s standing height scaled to `height` tiles
void Players::drawShiftFrame(const Character &sheet, const Animation &anim, int frame, float center_x, uint8_t alpha,
                             bool flip, float height, float lift)
{
    if (frame < 0 || sheet.sheet.getSize().x == 0)
        return;

    float scale = run.view.tile_size * (height > 0 ? height : TITAN_FORM_HEIGHT) / sheet.body;
    sf::Sprite sprite(sheet.sheet);

    sprite.setTextureRect(sheet.frameRect(anim, frame));
    sprite.setOrigin({sheet.frame_size.x / 2.f, sheet.feet});
    sprite.setScale({flip ? -scale : scale, scale});
    sprite.setPosition(run.toScreen({center_x, run.floor() - lift}));
    sprite.setColor(withAlpha(sf::Color::White, alpha));
    canvas.draw(sprite);
}
