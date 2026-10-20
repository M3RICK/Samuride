// What is drawn over the run: HUD, district banner, countdown, game over and results

#include "game_screen.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include <algorithm>
#include <format>

using namespace Palette;

static std::string playerLabel(int player_num, int me)
{
    return player_num == me ? "YOU" : "CHOOM";
}

//=============================================================================
// HUD
//=============================================================================

void GameScreen::drawHUD(const std::map<int, PlayerState> &players, int me, bool cyberpsycho)
{
    auto mine = run.display_positions.find(me);
    int distance = mine != run.display_positions.end() ? static_cast<int>(mine->second.x) : 0;
    float y = 90;

    canvas.text(std::format("{:04} M", distance), 56, NEON_YELLOW, {24, 16}, {0, 0});

    if (players.size() == 1 && best_distance > 0) {
        canvas.text(std::format("BEST {} M", best_distance), 24, DIM_WHITE, {24, y}, {0, 0});
        y += 36;
    }

    for (const auto &[player_num, player] : players) {
        drawHUDLine(player_num, player, me, y, cyberpsycho);
        y += 44;
    }

    if (cyberpsycho)
        canvas.text(std::format("DRONES DOWN  {}", drones_down), 26, NEON_RED, {24, y}, {0, 0});
    if (cyberpsycho && players.count(me) && ultimateOf(run.characterOf(me)) != Ultimate::None)
        drawUltimatePanel(players.at(me), me);
}

// Top right: your portrait, your ultimate's name and a segmented gauge in its colour. Full, it lights up
// and says which key fires it. Mikasa's thunder spears show there while she has some.
void GameScreen::drawUltimatePanel(const PlayerState &player, int me)
{
    static constexpr int SEGMENTS = 10;
    UltimateLook look = ultimateLook(ultimateOf(run.characterOf(me)));
    bool ready = player.ultimate >= 100;
    float pulse = 0.5f + 0.5f * std::sin(canvas.time() * 8);
    sf::FloatRect box({Screen::WIDTH - 24 - 520, 20}, {520, 110});

    if (ready && !ultimate_was_ready)
        audio.play(Audio::Sfx::UltimateReady);
    ultimate_was_ready = ready;

    canvas.card(box, ready);
    if (ready)
        canvas.halo(box.getCenter(), 300, withAlpha(look.color, static_cast<uint8_t>(25 + 30 * pulse)));
    canvas.image(sprites.character(run.characterOf(me)).portrait, box.position + sf::Vector2f(12, 12), 86, {0, 0},
                 sf::Color::White);

    sf::Vector2f text_pos = box.position + sf::Vector2f(116, 30);
    sf::Vector2f gauge_pos = box.position + sf::Vector2f(116, 62);
    float gauge_w = box.size.x - 116 - 20;

    if (player.spears > 0) {
        canvas.text(std::format("{}  [F]", look.name), 28, look.color, text_pos, {0, 0.5f});
        for (int i = 0; i < THUNDER_SPEARS; i++)
            drawSpearIcon(gauge_pos + sf::Vector2f(20 + i * 50, 0), 40, i < player.spears);
        return;
    }

    if (ready) {
        canvas.glitchText(std::format("[{}]  {}", ultimate_key, look.name), 30, look.color,
                          text_pos + sf::Vector2f(gauge_w / 2, 0));
    } else {
        canvas.text(look.name, 28, DIM_WHITE, text_pos, {0, 0.5f});
        canvas.text(std::format("{}%", player.ultimate), 24, HINT_GREY, box.position + sf::Vector2f(box.size.x - 20, 30),
                    {1, 0.5f});
    }

    float segment_w = gauge_w / SEGMENTS;
    for (int i = 0; i < SEGMENTS; i++) {
        bool lit = player.ultimate >= (i + 1) * 100 / SEGMENTS;
        sf::RectangleShape segment({segment_w - 6, 30});
        segment.setPosition(gauge_pos + sf::Vector2f(i * segment_w, 0));
        segment.setFillColor(lit ? withAlpha(look.color, ready ? static_cast<uint8_t>(170 + 85 * pulse) : 220)
                                 : sf::Color(255, 255, 255, 30));
        canvas.draw(segment);
    }
}

// A thunder spear standing up: shaft and head, hollow once thrown
void GameScreen::drawSpearIcon(sf::Vector2f pos, float height, bool loaded)
{
    sf::Color color = loaded ? NEON_CYAN : sf::Color(255, 255, 255, 50);

    sf::RectangleShape shaft({6, height * 0.7f});
    shaft.setPosition(pos + sf::Vector2f(-3, height * 0.3f));
    shaft.setFillColor(color);
    canvas.draw(shaft);

    sf::ConvexShape head(3);
    head.setPoint(0, pos);
    head.setPoint(1, pos + sf::Vector2f(-9, height * 0.32f));
    head.setPoint(2, pos + sf::Vector2f(9, height * 0.32f));
    head.setFillColor(loaded ? sf::Color::White : color);
    canvas.draw(head);
    if (loaded)
        canvas.halo(pos + sf::Vector2f(0, height / 2), height * 0.6f, withAlpha(NEON_CYAN, 50));
}

// An ultimate's screen flash, and its name across the middle of the screen for a moment
void GameScreen::drawFlashAndCallout()
{
    static constexpr float CALLOUT_TIME = 1.3f;
    float flash = 1 - run.flash.clock.getElapsedTime().asSeconds() / run.flash.duration;

    if (flash > 0 && run.flash.color.a > 0) {
        sf::RectangleShape cover({Screen::WIDTH, Screen::HEIGHT});
        cover.setFillColor(withAlpha(run.flash.color, static_cast<uint8_t>(200 * flash)));
        canvas.draw(cover, sf::BlendAdd);
    }

    float t = run.callout_clock.getElapsedTime().asSeconds() / CALLOUT_TIME;
    if (run.callout.empty() || t > 1)
        return;
    // Slams in big, settles, then fades
    float size = 150 - 40 * std::min(1.0f, t * 6);
    canvas.glitchText(run.callout, static_cast<unsigned>(size), withAlpha(run.callout_color, static_cast<uint8_t>(255 * (1 - t * t))),
                      Screen::CENTER + sf::Vector2f(0, -120));
}

// Portrait, then "YOU   12 EDDIES", a hexagon while shielded, and Lucy's RAM in CYBERPSYCHO
void GameScreen::drawHUDLine(int player_num, const PlayerState &player, int me, float y, bool cyberpsycho)
{
    std::string line = playerLabel(player_num, me) + "   " + std::to_string(player.score) + " EDDIES";
    sf::Text text = canvas.makeText(line, 28, player_num == me ? NEON_YELLOW : NEON_CYAN, {70, y}, {0, 0});

    canvas.image(sprites.character(run.characterOf(player_num)).portrait, {24, y - 2}, 34, {0, 0}, sf::Color::White);
    canvas.draw(text);

    if (player.shielded) {
        sf::FloatRect bounds = text.getGlobalBounds();
        canvas.hexagon({bounds.position.x + bounds.size.x + 26, bounds.getCenter().y}, 12, SHIELD_COLOR, 0);
    }

    if (!cyberpsycho || weaponOf(run.characterOf(player_num)) != Weapon::Hack)
        return;

    sf::FloatRect bounds = text.getGlobalBounds();
    float x = bounds.position.x + bounds.size.x + (player.shielded ? 52 : 20);
    for (int i = 0; i < RAM_MAX; i++) {
        sf::RectangleShape chip({14, 14});
        chip.setPosition({x + i * 22, bounds.getCenter().y - 7});
        chip.setFillColor(i < player.ram ? NEON_PURPLE : sf::Color::Transparent);
        chip.setOutlineColor(NEON_PURPLE);
        chip.setOutlineThickness(2);
        canvas.draw(chip);
    }
}

// Your timed power-ups drain a gauge each, stacked; the Sandevistan also tints the screen magenta
void GameScreen::drawPowerUps(const PlayerState &player, int me)
{
    const PlayerAnim &anim = run.player_anims[me];
    float y = 40;

    if (player.form == Form::Titan) {
        drawGauge("ATTACK TITAN", NEON_ORANGE, 1 - anim.form_clock.getElapsedTime().asSeconds() / TITAN_TIME, y);
        y += 80;
    } else if (player.sprinting) {
        sf::RectangleShape tint({Screen::WIDTH, Screen::HEIGHT});
        tint.setFillColor(withAlpha(SANDEVISTAN_COLOR, 22));
        canvas.draw(tint, sf::BlendAdd);
        drawGauge("SANDEVISTAN", SANDEVISTAN_COLOR, 1 - anim.sprint_clock.getElapsedTime().asSeconds() / SANDEVISTAN_TIME, y);
        y += 80;
    }
    if (player.berserk)
        drawGauge("BERSERK", BERSERK_COLOR, 1 - anim.berserk_clock.getElapsedTime().asSeconds() / BERSERK_TIME, y);
}

void GameScreen::drawGauge(const std::string &label, sf::Color color, float left, float y)
{
    sf::Vector2f pos(Screen::WIDTH / 2 - 250, y);

    sf::RectangleShape back({500, 18});
    back.setPosition(pos);
    back.setFillColor(sf::Color(0, 0, 0, 160));
    back.setOutlineColor(color);
    back.setOutlineThickness(2);
    canvas.draw(back);

    sf::RectangleShape fill({500 * std::clamp(left, 0.0f, 1.0f), 18});
    fill.setPosition(pos);
    fill.setFillColor(color);
    canvas.draw(fill);

    canvas.text(label, 26, color, pos + sf::Vector2f(250, 40));
}

// "ENTERING <district>", for a moment after the scenery changes
void GameScreen::drawBanner()
{
    if (banner_clock.getElapsedTime().asSeconds() >= BANNER_DURATION)
        return;

    canvas.glitchText(banner, 54, NEON_CYAN, {Screen::WIDTH / 2, 200});
    canvas.text(banner_tagline, 28, DIM_WHITE, {Screen::WIDTH / 2, 260});
}

void GameScreen::drawCountdown(Phase phase, int countdown)
{
    if (phase == Phase::Countdown && countdown > 0)
        canvas.glitchText(std::to_string(countdown), 220, NEON_YELLOW, Screen::CENTER);
    else if (phase == Phase::Running && phase_clock.getElapsedTime().asSeconds() < 1.2f)
        canvas.glitchText("BURN THE CITY", 120, NEON_RED, Screen::CENTER);
}

//=============================================================================
// Game Over
//=============================================================================

void GameScreen::drawGameOver(GameState &state, const std::map<int, PlayerState> &players, int me)
{
    bool solo = players.size() == 1;
    auto mine = players.find(me);

    canvas.overlay(190);

    if (solo && mine != players.end())
        drawSoloResults(mine->second);
    else
        drawVersusResults(players, state.getWinner(), me);

    canvas.text(std::format("WALLET  {} EDDIES", wallet), 28, NEON_PINK, Screen::CENTER + sf::Vector2f(0, 160));
    if (phase_clock.getElapsedTime().asSeconds() >= PROMPT_DELAY)
        drawRematchPrompt(state, solo);
}

void GameScreen::drawSoloResults(const PlayerState &me)
{
    sf::Vector2f center = Screen::CENTER;
    std::string result = std::format("{} M        {} EDDIES", static_cast<int>(me.x), me.score);

    canvas.glitchText("FLATLINED", 140, NEON_RED, center + sf::Vector2f(0, -150));
    canvas.text(result, 44, sf::Color::White, center + sf::Vector2f(0, 10));

    if (new_record)
        canvas.glitchText("NEW PERSONAL BEST", 48, NEON_YELLOW, center + sf::Vector2f(0, 95));
    else
        canvas.text(std::format("BEST  {} M", best_distance), 32, DIM_WHITE, center + sf::Vector2f(0, 95));
}

void GameScreen::drawVersusResults(const std::map<int, PlayerState> &players, int winner, int me)
{
    sf::Vector2f center = Screen::CENTER;
    sf::Vector2f title_pos = center + sf::Vector2f(0, -150);
    auto mine = players.find(me);
    int distance = mine != players.end() ? static_cast<int>(mine->second.x) : 0;

    if (winner == me)
        canvas.glitchText("LEGEND OF THE AFTERLIFE", 90, NEON_YELLOW, title_pos);
    else if (winner >= 0)
        canvas.glitchText("FLATLINED", 140, NEON_RED, title_pos);
    else
        canvas.glitchText("NO WINNERS IN NIGHT CITY", 90, NEON_CYAN, title_pos);

    std::string scores;
    for (const auto &[player_num, player] : players) {
        if (!scores.empty())
            scores += "        ";
        scores += playerLabel(player_num, me) + "  " + std::to_string(player.score) + " EDDIES";
    }

    canvas.text(std::format("{} M", distance), 44, sf::Color::White, center + sf::Vector2f(0, 10));
    canvas.text(scores, 34, DIM_WHITE, center + sf::Vector2f(0, 80));
}

void GameScreen::drawRematchPrompt(GameState &state, bool solo)
{
    sf::Vector2f pos = Screen::CENTER + sf::Vector2f(0, 220);

    if (state.isRematchRequested())
        canvas.text("WAITING FOR YOUR CHOOM...", 32, NEON_CYAN, pos);
    else
        canvas.text(solo ? "[SPACE] RUN AGAIN     [ESC] MENU" : "[SPACE] REMATCH     [ESC] MENU", 32, NEON_YELLOW, pos);
}

void GameScreen::drawConnectionLost()
{
    canvas.target().setView(canvas.screenView());
    canvas.overlay(200);

    canvas.glitchText("CONNECTION LOST", 110, NEON_RED, Screen::CENTER + sf::Vector2f(0, -40));
    canvas.text("[ESC] BACK TO MENU", 32, NEON_YELLOW, Screen::CENTER + sf::Vector2f(0, 100));
}
