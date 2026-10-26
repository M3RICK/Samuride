// Adam Smasher: his hulk, his rockets, his slam and his arm cannon, and his health bar.
// ponytail: plain shapes until Smasher gets his sprite sheet

#include "boss.hpp"
#include "canvas.hpp"
#include "run.hpp"
#include "../audio.hpp"
#include <cmath>

using namespace Palette;

static constexpr sf::Color STEEL{70, 74, 86};
static constexpr sf::Color DARK_STEEL{38, 40, 50};

Smasher::Smasher(Canvas &canvas, Run &run, Audio &audio) : canvas(canvas), run(run), audio(audio) {
}

static void rect(Canvas &canvas, sf::Vector2f pos, sf::Vector2f size, sf::Color color)
{
    sf::RectangleShape shape(size);
    shape.setPosition(pos);
    shape.setFillColor(color);
    canvas.draw(shape);
}

void Smasher::draw(const BossState &boss, bool cyberpsycho)
{
    if (boss.fighting && !last.fighting) {
        audio.play(Audio::Sfx::Roar);
        run.shakeAtLeast(0.6f, 14);
        run.callout = "ADAM SMASHER";
        run.callout_color = NEON_RED;
        run.callout_clock.restart();
    }
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
    // His x jumps when he leaps in for a slam and back: follow it in a few tenths of a second
    float dt = std::min(0.1f, frame_clock.restart().asSeconds());
    float ahead = boss.x - run.view.camera_x;
    if (!last.fighting)
        shown_x = ahead;
    shown_x += (ahead - shown_x) * std::min(1.0f, dt * 10);
    last = boss;
    if (!boss.fighting)
        return;

    drawAttack(boss);
    drawBody(boss);
    if (cyberpsycho)
        drawHealth(boss);
}

// A hulking cyborg standing on the floor, 7 tiles tall: legs, torso, head with a red visor, and the arm
// cannon pointing at the runners. He crouches before a slam, and is in the air while he leaps.
void Smasher::drawBody(const BossState &boss)
{
    float tile = run.view.tile_size;
    float floor = run.toScreen({0, run.floor()}).y;
    float left = run.toScreen({run.view.camera_x + shown_x - 0.5f, 0}).x;
    float leap = std::abs(boss.x - run.view.camera_x - shown_x);
    float bob = std::sin(canvas.time() * 3) * tile * 0.05f - std::min(leap, 4.0f) * tile * 0.6f;
    if (boss.attack == BossAttack::Slam && boss.phase == BossPhase::Warning)
        bob = tile * 0.3f;
    float top = floor - tile * 7 + bob;

    rect(canvas, {left + tile * 0.5f, top + tile * 4.5f}, {tile * 0.8f, floor - top - tile * 4.5f}, DARK_STEEL);
    rect(canvas, {left + tile * 1.6f, top + tile * 4.5f}, {tile * 0.8f, floor - top - tile * 4.5f}, DARK_STEEL);
    rect(canvas, {left, top + tile * 1.4f}, {tile * 3, tile * 3.3f}, STEEL);
    rect(canvas, {left + tile * 0.3f, top + tile * 2}, {tile * 2.4f, tile * 0.25f}, NEON_RED);
    rect(canvas, {left + tile * 0.8f, top}, {tile * 1.4f, tile * 1.4f}, STEEL);
    rect(canvas, {left + tile * 0.8f, top + tile * 0.5f}, {tile * 1.4f, tile * 0.3f}, withAlpha(NEON_RED, canvas.blink() ? 255 : 180));

    // The cannon arm, raised to the cannon's line while it aims
    float arm_y = top + tile * 2.6f;
    if (boss.attack == BossAttack::Cannon)
        arm_y = run.toScreen({0, boss.y}).y - tile * 0.35f;
    rect(canvas, {left - tile * 1.6f, arm_y}, {tile * 2, tile * 0.7f}, DARK_STEEL);
    rect(canvas, {left - tile * 1.8f, arm_y + tile * 0.15f}, {tile * 0.3f, tile * 0.4f}, NEON_ORANGE);
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
