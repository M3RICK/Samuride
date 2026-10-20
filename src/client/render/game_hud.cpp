// What is drawn over the run: HUD, district banner, countdown, game over and results

#include "game_screen.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include "../touch.hpp"
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

    if (players.size() == 1 && bests.distance(cyberpsycho) > 0) {
        canvas.text(std::format("BEST {} M", bests.distance(cyberpsycho)), 24, DIM_WHITE, {24, y}, {0, 0});
        y += 36;
    }

    for (const auto &[player_num, player] : players) {
        drawHUDLine(player_num, player, me, y);
        y += 44;
    }

    if (cyberpsycho)
        canvas.text(players.size() == 1 && bests.drones > 0 ? std::format("DRONES DOWN  {}   BEST {}", drones_down, bests.drones)
                                                            : std::format("DRONES DOWN  {}", drones_down),
                    26, NEON_RED, {24, y}, {0, 0});
    if (players.count(me))
        drawRunnerPanel(players.at(me), me, cyberpsycho && ultimateOf(run.characterOf(me)) != Ultimate::None);
    if (Touch::ENABLED && cyberpsycho && players.count(me))
        drawTouchButtons(players.at(me), me);
}

// Phones: attack, and the ultimate above it, lit in its colour once ready
void GameScreen::drawTouchButtons(const PlayerState &player, int me)
{
    Ultimate ultimate = ultimateOf(run.characterOf(me));
    bool ready = player.ultimate >= 100;
    float pulse = 0.5f + 0.5f * std::sin(canvas.time() * 8);

    auto button = [&](sf::Vector2f center, sf::Color color, bool lit, const std::string &label) {
        sf::CircleShape circle(Touch::BUTTON_RADIUS);
        circle.setOrigin({Touch::BUTTON_RADIUS, Touch::BUTTON_RADIUS});
        circle.setPosition(center);
        circle.setFillColor(withAlpha(color, lit ? 90 : 25));
        circle.setOutlineColor(withAlpha(color, lit ? 255 : 120));
        circle.setOutlineThickness(4);
        canvas.draw(circle);
        canvas.text(label, 30, withAlpha(color, lit ? 255 : 150), center);
    };

    button(Touch::ATTACK, NEON_RED, true, "ATTACK");
    if (ultimate != Ultimate::None) {
        sf::Color color = ultimateLook(ultimate).color;
        bool spears = player.spears > 0;
        button(Touch::ULTIMATE, color, ready || spears, spears ? "SPEAR" : ready ? "ULT" : std::format("{}%", player.ultimate));
        if (ready || spears)
            canvas.halo(Touch::ULTIMATE, Touch::BUTTON_RADIUS * (1.2f + 0.2f * pulse), withAlpha(color, 60));
    }
}

// Bottom centre: your runner's face in a round frame. In CYBERPSYCHO a ring around it fills with your
// ultimate in its colour; full, it pulses and says which key fires it. While Mikasa holds thunder spears,
// the ring shows one notch per spear.
void GameScreen::drawRunnerPanel(const PlayerState &player, int me, bool ultimate)
{
    static constexpr float FACE = 56;
    static constexpr float RING = 68;
    UltimateLook look = ultimateLook(ultimateOf(run.characterOf(me)));
    bool ready = player.ultimate >= 100;
    // Low, mostly in the strip under the play area: it must not hide a trap on the floor
    sf::Vector2f center(Screen::WIDTH / 2, Screen::HEIGHT - 6 - RING);

    // Fired: the gauge empties, or a spear leaves
    if ((ultimate_was_ready && !ready) || player.spears < spears_shown)
        react(Mood::Shout);
    if (ready && !ultimate_was_ready)
        audio.play(Audio::Sfx::UltimateReady);
    ultimate_was_ready = ready;
    spears_shown = player.spears;

    canvas.backing(center, RING, ultimate ? sf::Color::Transparent : DIM_WHITE);
    if (ultimate && player.spears > 0)
        drawRing(center, RING, THUNDER_SPEARS, static_cast<float>(player.spears) / THUNDER_SPEARS, NEON_CYAN, false);
    else if (ultimate)
        drawRing(center, RING, 1, player.ultimate / 100.f, look.color, ready);
    drawFace(player, me, center, FACE);

    if (!ultimate || (!ready && player.spears == 0))
        return;
    std::string label = Touch::ENABLED ? std::string(look.name) : std::format("[{}]  {}", ultimate_key, look.name);
    sf::Vector2f label_pos = center - sf::Vector2f(0, RING + 30);
    if (player.spears > 0)
        canvas.text(label, 28, NEON_CYAN, label_pos);
    else
        canvas.glitchText(label, 30, look.color, label_pos);
}

// The gauge around the face, clockwise from the top, the `lit` share of it in colour.
// Cut into notches when there is more than one segment.
void GameScreen::drawRing(sf::Vector2f center, float radius, int segments, float lit, sf::Color color, bool pulsing)
{
    static constexpr float THICKNESS = 10;
    float gap = segments > 1 ? 6 : 0;
    float span = 360.f / segments;
    float pulse = 0.5f + 0.5f * std::sin(canvas.time() * 8);
    sf::Color fill = pulsing ? withAlpha(color, static_cast<uint8_t>(170 + 85 * pulse)) : color;

    if (pulsing)
        canvas.halo(center, radius * (1.2f + 0.1f * pulse), withAlpha(color, static_cast<uint8_t>(30 + 30 * pulse)));
    for (int i = 0; i < segments; i++) {
        float from = i * span + gap / 2;
        float to = (i + 1) * span - gap / 2;
        float end = std::clamp(lit * 360, from, to);
        canvas.arc(center, radius - THICKNESS, radius, from, to, sf::Color(255, 255, 255, 55));
        if (end > from)
            canvas.arc(center, radius - THICKNESS, radius, from, end, fill);
    }
}

// A mood raised by the run. A lesser one doesn't cut a stronger one short, and kills in a quick row
// work up to a battle cry.
void GameScreen::react(Mood new_mood)
{
    static constexpr float STREAK_TIME = 1.5f;
    float age = mood_clock.getElapsedTime().asSeconds();

    if (new_mood == Mood::Kill) {
        kill_streak = (mood == Mood::Kill || mood == Mood::Frenzy) && age < STREAK_TIME ? kill_streak + 1 : 1;
        if (kill_streak >= 3)
            new_mood = Mood::Frenzy;
    }
    if (new_mood < mood && age < 0.6f)
        return;
    mood = new_mood;
    mood_clock.restart();
}

// Which head of the faces sheet shows now, Doom style. Mikasa's sheet, by rows: her head turning from
// facing you to profile (0, then 1), a cry closing to a frown (2), bleeding with her eyes closing (3),
// tired (4), a smile (5).
sf::Vector2i GameScreen::faceCell(const PlayerState &player, int me)
{
    static constexpr float TURN_STEP = 0.05f;
    static const std::map<Mood, float> DURATION = {
        {Mood::Pickup, 1.2f}, {Mood::Kill, 0.6f}, {Mood::Frenzy, 1.0f}, {Mood::Surprised, 1.0f}, {Mood::Shout, 1.2f}};
    float now = canvas.time();
    float age = mood_clock.getElapsedTime().asSeconds();
    // Frames `first` to `last` of a row at `fps`, held on the last
    auto play = [](int row, int first, int last, float fps, float t) {
        return sf::Vector2i{std::min(last, first + static_cast<int>(t * fps)), row};
    };
    bool flap = static_cast<int>(now * 9) % 2 == 0;     // the mouth, while she shouts

    if (run.death_clocks.count(me))
        return play(3, 0, 6, 6, run.death_clocks.at(me).getElapsedTime().asSeconds());
    if (mood != Mood::Idle && age < DURATION.at(mood)) {
        switch (mood) {
        case Mood::Shout:     return age < 0.2f ? play(2, 0, 3, 20, age) : sf::Vector2i{flap ? 3 : 2, 2};
        case Mood::Frenzy:    return {flap ? 1 : 0, 2};
        case Mood::Kill:      return {kill_streak % 2 ? 5 : 6, 2};
        case Mood::Surprised: return age < 0.4f ? sf::Vector2i{0, 2} : sf::Vector2i{0, 1};
        case Mood::Pickup:    return play(5, 2, 4, 8, age);
        case Mood::Idle:      break;
        }
    }
    if (player.form == Form::Titan || player.berserk)
        return {flap && static_cast<int>(now) % 3 == 0 ? 1 : 6, 2};
    if (titan_coming)
        return {8, 1};      // glaring at it, ahead
    if (player.sprinting)
        return {5, 1};
    if (player.ultimate >= 100)
        return {3, 0};

    // Idle: mostly facing you, now and then a look ahead, short or all the way to profile
    if (now >= head_until && head == head_target) {
        bool look_away = head_target == 0 && randf(0, 1) < 0.6f;
        head_target = look_away ? static_cast<int>(randf(2, 8.99f)) : 0;
        head_until = now + (look_away ? randf(0.6f, 1.4f) : randf(1.8f, 4.f));
    }
    if (head != head_target && now - head_moved >= TURN_STEP) {
        head += head < head_target ? 1 : -1;
        head_moved = now;
    }
    return {head, 0};
}

// Your face in a disc. A new mood pops it out a little, a scare shakes it. Runners without a faces
// sheet show their portrait.
void GameScreen::drawFace(const PlayerState &player, int me, sf::Vector2f center, float radius)
{
    const Character &character = sprites.character(run.characterOf(me));
    sf::CircleShape face(radius, 64);
    float age = mood_clock.getElapsedTime().asSeconds();

    face.setOrigin({radius, radius});
    face.setPosition(center);
    face.setScale(sf::Vector2f(1, 1) * (1 + 0.12f * std::max(0.f, 1 - age / 0.15f)));
    if (mood == Mood::Surprised && age < 0.3f)
        face.move({std::sin(age * 80) * 6 * (1 - age / 0.3f), 0});

    if (character.face_grid.x == 0) {
        face.setTexture(&character.portrait);
    } else {
        // The head without the shoulders: a square from near the top of the cell
        sf::Vector2i cell_size(character.faces.getSize().x / character.face_grid.x,
                               character.faces.getSize().y / character.face_grid.y);
        int side = std::min(cell_size.x, cell_size.y) * 4 / 5;
        sf::Vector2i cell = faceCell(player, me);
        face.setTexture(&character.faces);
        face.setTextureRect(sf::IntRect(cell.componentWiseMul(cell_size) + sf::Vector2i((cell_size.x - side) / 2, cell_size.y / 10),
                                        {side, side}));
    }
    canvas.draw(face);
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
    // Slams in big, then settles and fades. A few fixed sizes: each new font size renders its letters
    // again, which a size changing every frame made phones stutter on.
    unsigned int size = t < 0.06f ? 150 : t < 0.12f ? 130 : 110;
    canvas.glitchText(run.callout, size, withAlpha(run.callout_color, static_cast<uint8_t>(255 * (1 - t * t))),
                      Screen::CENTER + sf::Vector2f(0, -120));
}

// Portrait, then "YOU   12 EDDIES", and a hexagon while shielded (Lucy's RAM is over her head, players.cpp)
void GameScreen::drawHUDLine(int player_num, const PlayerState &player, int me, float y)
{
    std::string line = playerLabel(player_num, me) + "   " + std::to_string(player.score) + " EDDIES";
    sf::Text text = canvas.makeText(line, 28, player_num == me ? NEON_YELLOW : NEON_CYAN, {70, y}, {0, 0});

    canvas.image(sprites.character(run.characterOf(player_num)).portrait, {24, y - 2}, 34, {0, 0}, sf::Color::White);
    canvas.draw(text);

    if (player.shielded) {
        sf::FloatRect bounds = text.getGlobalBounds();
        canvas.hexagon({bounds.position.x + bounds.size.x + 26, bounds.getCenter().y}, 12, SHIELD_COLOR, 0);
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
        drawGauge("SANDEVISTAN", SANDEVISTAN_COLOR, 1 - anim.sprint_clock.getElapsedTime().asSeconds() / sandevistan_time, y);
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
    std::string result = cyberpsycho_run
                             ? std::format("{} M      {} DRONES      {} EDDIES", static_cast<int>(me.x), drones_down, me.score)
                             : std::format("{} M        {} EDDIES", static_cast<int>(me.x), me.score);

    canvas.glitchText("FLATLINED", 140, NEON_RED, center + sf::Vector2f(0, -150));
    canvas.text(result, 44, sf::Color::White, center + sf::Vector2f(0, 10));

    if (new_record || new_drone_record)
        canvas.glitchText(new_record && new_drone_record ? "NEW PERSONAL BESTS" : new_record ? "NEW PERSONAL BEST" : "NEW DRONE RECORD",
                          48, NEON_YELLOW, center + sf::Vector2f(0, 95));
    else if (cyberpsycho_run)
        canvas.text(std::format("BEST  {} M      {} DRONES", bests.cyberpsycho, bests.drones), 32, DIM_WHITE,
                    center + sf::Vector2f(0, 95));
    else
        canvas.text(std::format("BEST  {} M", bests.classic), 32, DIM_WHITE, center + sf::Vector2f(0, 95));
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
        canvas.text(solo ? Touch::hint("[SPACE] RUN AGAIN     [ESC] MENU", "TAP TO RUN AGAIN")
                         : Touch::hint("[SPACE] REMATCH     [ESC] MENU", "TAP TO REMATCH"),
                    32, NEON_YELLOW, pos);
    drawMenuButton();
}

// Phones: the way back to the menu
void GameScreen::drawMenuButton()
{
    if (!Touch::ENABLED)
        return;
    sf::FloatRect box(Touch::MENU_BUTTON - Touch::MENU_BUTTON_SIZE / 2.f, Touch::MENU_BUTTON_SIZE);
    canvas.card(box, true);
    canvas.text("MENU", 40, NEON_YELLOW, Touch::MENU_BUTTON);
}

void GameScreen::drawConnectionLost()
{
    canvas.target().setView(canvas.screenView());
    canvas.overlay(200);

    canvas.glitchText("CONNECTION LOST", 110, NEON_RED, Screen::CENTER + sf::Vector2f(0, -40));
    if (Touch::ENABLED)
        drawMenuButton();
    else
        canvas.text("[ESC] BACK TO MENU", 32, NEON_YELLOW, Screen::CENTER + sf::Vector2f(0, 100));
}
