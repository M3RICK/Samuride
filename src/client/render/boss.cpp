// Adam Smasher: his sprite, his rockets, his slam and his arm cannon, and his health bar

#include "boss.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include <algorithm>
#include <cmath>

using namespace Palette;

static constexpr sf::Color DARK_STEEL{38, 40, 50};

Smasher::Smasher(Canvas &canvas, const Sprites &sprites, Run &run, Audio &audio)
    : canvas(canvas), sprites(sprites), run(run), audio(audio) {
}

static void rect(Canvas &canvas, sf::Vector2f pos, sf::Vector2f size, sf::Color color)
{
    sf::RectangleShape shape(size);
    shape.setPosition(pos);
    shape.setFillColor(color);
    canvas.draw(shape);
}

// What changed since last frame: his entrance, an attack striking, a hit, his death
void Smasher::react(const BossState &boss)
{
    if (boss.fighting && !last.fighting) {
        audio.play(Audio::Sfx::Roar);
        run.shakeAtLeast(0.6f, 14);
        run.callout = "ADAM SMASHER";
        run.callout_color = NEON_RED;
        run.callout_clock.restart();
        since_entrance.restart();
    }
    if (!boss.fighting && last.fighting && boss.health == 0) {
        death_x = run.view.camera_x + shown_x;
        since_death.restart();
    }
    if (boss.fighting && boss.health < last.health)
        since_hit.restart();
    bool striking = boss.phase == BossPhase::Strike;
    if (boss.fighting && striking && (last.phase != BossPhase::Strike || last.attack != boss.attack)) {
        if (boss.attack == BossAttack::Rockets)
            audio.play(Audio::Sfx::Rocket);
        if (boss.attack == BossAttack::Slam) {
            audio.play(Audio::Sfx::Stomp);
            run.shakeAtLeast(0.4f, 18);
        }
        if (boss.attack == BossAttack::Cannon) {
            audio.play(Audio::Sfx::Explosion);
            run.shakeAtLeast(0.25f);
        }
    }
}

void Smasher::draw(const BossState &boss, bool cyberpsycho)
{
    react(boss);

    // His x jumps when he leaps in for a slam and back: follow it in a few tenths of a second
    float dt = std::min(0.1f, frame_clock.restart().asSeconds());
    float ahead = boss.x - run.view.camera_x;
    if (!last.fighting)
        shown_x = ahead;
    shown_x += (ahead - shown_x) * std::min(1.0f, dt * 10);
    last = boss;

    const Character &look = sprites.smasher;
    if (death_x >= 0) {
        float t = since_death.getElapsedTime().asSeconds();
        float fade = std::clamp(2 - t / look.death.duration(), 0.0f, 1.0f);
        if (fade > 0)
            drawBody(look.death, look.death.frameAt(t), death_x, 0, withAlpha(sf::Color::White, static_cast<uint8_t>(255 * fade)));
    }
    if (!boss.fighting)
        return;

    drawAttack(boss);
    auto [anim, frame] = pose(boss);
    float leap = std::min(std::abs(ahead - shown_x), 4.0f) * 0.6f;
    bool hurt = since_hit.getElapsedTime().asSeconds() < 0.08f;
    drawBody(*anim, frame, run.view.camera_x + shown_x, leap, hurt ? sf::Color(255, 120, 120) : sf::Color::White);
    if (cyberpsycho)
        drawHealth(boss);
}

// Each attack's frames follow its phases: the slam lands and the cannon fires when the server strikes
std::pair<const Animation *, int> Smasher::pose(const BossState &boss) const
{
    const Character &look = sprites.smasher;
    float t = boss.time;
    auto frames = [](const Animation &anim, float from, float to, float share) {
        return std::pair{&anim, static_cast<int>(from + (to - from) * std::clamp(share, 0.0f, 0.999f))};
    };

    switch (boss.attack) {
        case BossAttack::Rockets:
            // The pods open during the warning, the rockets leave as it strikes
            if (boss.phase == BossPhase::Warning)
                return frames(look.rockets, 0, 3, t / BOSS_ROCKET_WARN);
            return {&look.rockets, look.rockets.frameAt(t - BOSS_ROCKET_WARN + 3 / look.rockets.fps)};
        case BossAttack::Slam:
            // Crouch and leap during the warning, the impact at the strike, then getting back up
            if (boss.phase == BossPhase::Warning)
                return frames(look.slam, 0, 4, t / BOSS_SLAM_WARN);
            return frames(look.slam, 4, static_cast<float>(look.slam.frames),
                          (t - BOSS_SLAM_WARN) / (BOSS_SLAM_TIME + BOSS_SLAM_RECOVER));
        case BossAttack::Cannon:
            // Raised while it tracks, charging while it aims, the blast as it fires
            if (boss.phase == BossPhase::Warning)
                return frames(look.cannon, 0, 3, t / BOSS_CANNON_TRACK);
            if (boss.phase == BossPhase::Aim)
                return frames(look.cannon, 3, 5, (t - BOSS_CANNON_TRACK) / BOSS_CANNON_AIM);
            return {&look.cannon, look.cannon.frames - 1};
        default:
            break;
    }
    float entering = since_entrance.getElapsedTime().asSeconds();
    if (entering < look.entrance.duration())
        return {&look.entrance, look.entrance.frameAt(entering)};
    float hit = since_hit.getElapsedTime().asSeconds();
    if (hit < look.hit.duration())
        return {&look.hit, look.hit.frameAt(hit)};
    return {&look.run, look.run.frameAt(canvas.time())};
}

// Feet on the floor, HEIGHT tiles tall, his body from x - 0.5 to x + 2.5 like the server's
void Smasher::drawBody(const Animation &anim, int frame, float x, float lift, sf::Color color)
{
    const Character &look = sprites.smasher;
    if (look.sheet.getSize().x == 0 || frame < 0)
        return;
    float tile = run.view.tile_size;
    float scale = tile * HEIGHT / look.body;
    sf::Sprite sprite(look.sheet);
    sprite.setTextureRect(look.frameRect(anim, frame));
    sprite.setOrigin({look.frame_size.x / 2.f, look.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(run.toScreen({x + 1, run.floor() - lift}));
    sprite.setColor(color);
    canvas.draw(sprite);
}

void Smasher::drawAttack(const BossState &boss)
{
    float tile = run.view.tile_size;
    bool warning = boss.phase != BossPhase::Strike;
    int rows = static_cast<int>(run.floor());

    if (boss.attack == BossAttack::Rockets) {
        float screen_x = warning ? Screen::WIDTH - tile * 1.5f : run.toScreen({boss.rockets_x, 0}).x;
        for (int row = 0; row < rows; row++) {
            if (!rocketRow(row, boss.y))
                continue;
            float y = run.toScreen({0, row + 0.5f}).y;
            if (warning) {
                // Chevrons blinking on the right edge, one per rocket to come
                if (canvas.blink())
                    canvas.text("<<", 44, NEON_RED, {screen_x, y});
                continue;
            }
            rect(canvas, {screen_x, y - tile * 0.15f}, {tile * 0.9f, tile * 0.3f}, sf::Color(200, 200, 210));
            rect(canvas, {screen_x - tile * 0.1f, y - tile * 0.2f}, {tile * 0.2f, tile * 0.4f}, NEON_RED);
            rect(canvas, {screen_x + tile * 0.9f, y - tile * 0.12f}, {tile * (0.6f + 0.3f * canvas.blink()), tile * 0.24f}, NEON_ORANGE);
        }
    } else if (boss.attack == BossAttack::Slam) {
        float top = run.toScreen({0, run.floor() - BOSS_SLAM_HEIGHT}).y;
        float height = run.toScreen({0, run.floor()}).y - top;
        if (warning) {
            if (canvas.blink())
                rect(canvas, {0, top}, {Screen::WIDTH, height}, withAlpha(NEON_RED, 70));
            rect(canvas, {0, top}, {Screen::WIDTH, 4}, withAlpha(NEON_RED, 200));
        } else {
            rect(canvas, {0, top}, {Screen::WIDTH, height}, withAlpha(NEON_ORANGE, 150));
            rect(canvas, {0, top + height * 0.4f}, {Screen::WIDTH, height * 0.2f}, withAlpha(sf::Color::White, 200));
        }
    } else if (boss.attack == BossAttack::Cannon) {
        float y = run.toScreen({0, boss.y}).y;
        if (boss.phase == BossPhase::Strike) {
            for (auto [height, color] : {std::pair{tile * BOSS_CANNON_HALF * 2.5f, withAlpha(NEON_ORANGE, 90)},
                                         std::pair{tile * BOSS_CANNON_HALF * 2, NEON_ORANGE},
                                         std::pair{tile * BOSS_CANNON_HALF, sf::Color::White}})
                rect(canvas, {0, y - height / 2}, {Screen::WIDTH, height}, color);
        } else if (boss.phase == BossPhase::Warning || canvas.blink()) {
            float thickness = boss.phase == BossPhase::Aim ? 8 : 3;
            rect(canvas, {0, y - thickness / 2}, {Screen::WIDTH, thickness}, withAlpha(NEON_ORANGE, 200));
        }
    }
}

// Across the top of the screen, under his name
void Smasher::drawHealth(const BossState &boss)
{
    static constexpr float WIDTH = 800;
    sf::Vector2f pos(Screen::WIDTH / 2 - WIDTH / 2, 120);

    canvas.text("ADAM SMASHER", 30, NEON_RED, {Screen::WIDTH / 2, pos.y - 24});
    rect(canvas, pos, {WIDTH, 18}, withAlpha(DARK_STEEL, 220));
    rect(canvas, pos, {WIDTH * boss.health / 100.f, 18}, NEON_RED);
}
