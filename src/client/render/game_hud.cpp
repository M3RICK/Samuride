// What is drawn over the run: HUD, district banner, countdown, game over and results

#include "game_screen.hpp"
#include "sprites.hpp"
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

// Your timed power-ups drain a gauge; the Sandevistan also tints the screen magenta
void GameScreen::drawPowerUps(const PlayerState &player, int me)
{
    const PlayerAnim &anim = run.player_anims[me];

    if (player.form == Form::Titan) {
        drawGauge("ATTACK TITAN", NEON_ORANGE, 1 - anim.form_clock.getElapsedTime().asSeconds() / TITAN_TIME);
    } else if (player.sprinting) {
        sf::RectangleShape tint({Screen::WIDTH, Screen::HEIGHT});
        tint.setFillColor(withAlpha(SANDEVISTAN_COLOR, 22));
        canvas.draw(tint, sf::BlendAdd);
        drawGauge("SANDEVISTAN", SANDEVISTAN_COLOR, 1 - anim.sprint_clock.getElapsedTime().asSeconds() / SANDEVISTAN_TIME);
    }
}

void GameScreen::drawGauge(const std::string &label, sf::Color color, float left)
{
    sf::Vector2f pos(Screen::WIDTH / 2 - 250, 40);

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
