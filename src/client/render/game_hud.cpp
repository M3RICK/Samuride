// What is drawn over the run: HUD, district banner, countdown, game over and results

#include "game_screen.hpp"
#include "sprites.hpp"
#include "../audio.hpp"
#include "../touch.hpp"
#include "../../common/tuning.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <map>
#include <vector>

using namespace Palette;

static std::string playerLabel(int player_num, int me)
{
    return player_num == me ? "YOU" : "CHOOM";
}

// "LUCKY CHIP II"; the one-level upgrades go without a number
static std::string upgradeLabel(int upgrade, int level)
{
    static const char *numerals[] = {"", "I", "II", "III", "IV", "V"};
    std::string name = UPGRADES[upgrade].name;

    return UPGRADES[upgrade].max_level > 1 ? name + " " + numerals[std::clamp(level, 0, 5)] : name;
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
    // assets/tuning.txt changed something: screenshots and records then say so
    if (Tuning::custom()) {
        canvas.text("CUSTOM TUNING", 22, NEON_CYAN, {24, y}, {0, 0});
        y += 32;
    }

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
    if (cyberpsycho && players.count(me))
        drawWanted(players.at(me), me, y + 40);
    if (players.count(me))
        drawRunnerPanel(players.at(me), me, cyberpsycho && ultimateOf(run.characterOf(me)) != Ultimate::None);
    if (players.count(me) && cyberpsycho && weaponOf(run.characterOf(me)) == Weapon::Rebellion)
        drawStyle(players.at(me));
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

// Dante's style rank, right of his face like DMC's: a big letter in the colour of the rank and its word.
// Each rank up slams the letter in with a flash and a power chord, the word sliding in after it.
void GameScreen::drawStyle(const PlayerState &player)
{
    static const sf::Color COLORS[STYLE_RANKS] = {DIM_WHITE, NEON_CYAN, NEON_CYAN, NEON_YELLOW, NEON_ORANGE,
                                                  NEON_ORANGE, NEON_RED};
    static const char *WORDS[STYLE_RANKS] = {"Dismal", "Crazy!", "Badass!", "Apocalyptic!", "Savage!",
                                             "Sick Skills!!", "Smokin' Sexy Style!!!"};
    int rank = std::clamp(player.style, 0, STYLE_RANKS - 1);

    if (rank > style_shown) {
        audio.play(Audio::Sfx::StyleUp);
        style_clock.restart();
        if (rank >= 4)
            run.shakeAtLeast(0.15f, 6);
    }
    style_shown = rank;

    float t = style_clock.getElapsedTime().asSeconds();
    float slam = std::max(0.0f, 1 - t * 5);     // 0.2 s from huge down to its size
    float flash = std::max(0.0f, 1 - t * 2.5f);
    float slide = std::max(0.0f, 1 - t * 4);
    sf::Vector2f center(Screen::WIDTH / 2 + 190, Screen::HEIGHT - 80);
    unsigned size = static_cast<unsigned>((64 + 6 * rank) * (1 + 1.5f * slam * slam));

    if (flash > 0)
        canvas.halo(center, 50 + 110 * (1 - flash), withAlpha(COLORS[rank], static_cast<uint8_t>(140 * flash)));
    if (rank >= 4)
        canvas.glitchText(STYLE_NAMES[rank], size, COLORS[rank], center);
    else
        canvas.text(STYLE_NAMES[rank], size, COLORS[rank], center);
    if (rank > 0)
        canvas.text(WORDS[rank], 26 + 2 * rank, COLORS[rank], center + sf::Vector2f(60 + 260 * slide * slide, 12), {0, 0.5f});
}

// The NCPD wanted level under the drone count, GTA style: five stars, the lit ones flashing red and
// blue like a patrol car's lights, the bounty they pay beside them. A siren when one comes on.
void GameScreen::drawWanted(const PlayerState &player, int me, float y)
{
    static constexpr float RADIUS = 13;
    int stars = std::clamp(player.wanted, 0, WANTED_MAX_STARS);

    if (stars > wanted_shown) {
        audio.play(Audio::Sfx::Siren);
        run.callout = std::format("WANTED {}", std::string(stars, '*'));
        run.callout_color = NEON_RED;
        run.callout_clock.restart();
    }
    wanted_shown = stars;

    bool red = std::fmod(canvas.time(), 0.5f) < 0.25f;
    for (int i = 0; i < WANTED_MAX_STARS; i++) {
        sf::ConvexShape star(10);
        for (int point = 0; point < 10; point++) {
            float angle = point * 3.14159265f / 5 - 3.14159265f / 2;
            float r = point % 2 ? RADIUS * 0.45f : RADIUS;
            star.setPoint(point, {r * std::cos(angle), r * std::sin(angle)});
        }
        star.setPosition({24 + RADIUS + i * RADIUS * 2.3f, y + RADIUS});
        bool lit = i < stars;
        star.setFillColor(lit ? ((i % 2 == 0) == red ? NEON_RED : NEON_CYAN) : sf::Color(255, 255, 255, 40));
        canvas.draw(star);
    }
    if (stars > 0)
        if (implant > 0)
        canvas.text(IMPLANT_LOOKS[implant].name, 22, NEON_CYAN, {24, y + 2 * RADIUS + 22}, {0, 0.5f});
    if (boon != Boon::None)
        canvas.text(boonLook(boon, weaponOf(run.characterOf(me))).name, 22, NEON_YELLOW,
                    {24, y + 2 * RADIUS + (implant > 0 ? 52 : 22)}, {0, 0.5f});
    canvas.text(std::format("x{:.1f} EDDIES", 1 + WANTED_BOUNTY_PER_STAR * stars), 22, DIM_WHITE,
                    {24 + WANTED_MAX_STARS * RADIUS * 2.3f + 10, y + RADIUS}, {0, 0.5f});
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

// A look on a faces sheet: its heads, played at `fps`, looping or held on the last
struct FaceClip {
    std::vector<sf::Vector2i> cells;
    float fps = 1;
    bool loop = false;

    sf::Vector2i at(float seconds) const
    {
        int i = static_cast<int>(seconds * fps);
        return cells[loop ? i % cells.size() : std::min<size_t>(i, cells.size() - 1)];
    }
};

// Each runner's sheet has its own looks; their first row is always the head turning from facing you
// to profile, the idle face's.
struct FaceLooks {
    FaceClip death, shout, frenzy, kill, surprised, pickup, rage, titan, sprint, ready;
};

static const FaceLooks *faceLooks(const std::string &name)
{
    static const std::map<std::string, FaceLooks> LOOKS = {
        // A cry, a frown on a kill, a smile; bleeding with her eyes closing
        {"MIKASA", {.death = {{{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}}, 6},
                    .shout = {{{2, 2}, {3, 2}}, 9, true},
                    .frenzy = {{{0, 2}, {1, 2}}, 9, true},
                    .kill = {{{6, 2}, {5, 2}}},
                    .surprised = {{{0, 2}, {0, 1}}, 2.5f},
                    .pickup = {{{2, 5}, {3, 5}, {4, 5}}, 8},
                    .rage = {{{6, 2}, {6, 2}, {1, 2}}, 2, true},
                    .titan = {{{8, 1}}}, .sprint = {{{5, 1}}}, .ready = {{{3, 0}}}}},
        // She laughs: a grin on a kill, a mad cackle on a streak, a wink at a power-up
        {"JINX", {.death = {{{1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {6, 3}}, 6},
                  .shout = {{{3, 2}, {4, 2}}, 9, true},
                  .frenzy = {{{1, 2}, {2, 2}, {5, 2}, {6, 2}}, 9, true},
                  .kill = {{{0, 3}, {0, 2}}},
                  .surprised = {{{1, 3}, {7, 3}}, 2.5f},
                  .pickup = {{{0, 4}, {0, 3}}, 3},
                  .rage = {{{1, 2}, {2, 2}}, 6, true},
                  .titan = {{{8, 0}}}, .sprint = {{{5, 1}}}, .ready = {{{3, 0}}}}},
    };
    auto it = LOOKS.find(name);
    return it != LOOKS.end() ? &it->second : nullptr;
}

// Which head of your faces sheet shows now, Doom style
sf::Vector2i GameScreen::faceCell(const PlayerState &player, int me)
{
    static constexpr float TURN_STEP = 0.05f;
    static const std::map<Mood, float> DURATION = {
        {Mood::Pickup, 1.2f}, {Mood::Kill, 0.6f}, {Mood::Frenzy, 1.0f}, {Mood::Surprised, 1.0f}, {Mood::Shout, 1.2f}};
    const Character &character = sprites.character(run.characterOf(me));
    const FaceLooks *looks = faceLooks(character.name);
    float now = canvas.time();
    float age = mood_clock.getElapsedTime().asSeconds();

    if (looks && run.death_clocks.count(me))
        return looks->death.at(run.death_clocks.at(me).getElapsedTime().asSeconds());
    if (looks && mood != Mood::Idle && age < DURATION.at(mood)) {
        switch (mood) {
        case Mood::Shout:     return looks->shout.at(age);
        case Mood::Frenzy:    return looks->frenzy.at(age);
        case Mood::Kill:      return looks->kill.cells[kill_streak % looks->kill.cells.size()];
        case Mood::Surprised: return looks->surprised.at(age);
        case Mood::Pickup:    return looks->pickup.at(age);
        case Mood::Idle:      break;
        }
    }
    if (looks && (player.form == Form::Titan || player.berserk))
        return looks->rage.at(now);
    if (looks && titan_coming)
        return looks->titan.at(0);
    if (looks && player.sprinting)
        return looks->sprint.at(0);
    if (looks && player.ultimate >= 100)
        return looks->ready.at(0);

    // Idle: mostly facing you, now and then a look ahead, short or all the way to profile
    if (now >= head_until && head == head_target) {
        bool look_away = head_target == 0 && randf(0, 1) < 0.6f;
        head_target = look_away ? static_cast<int>(randf(2, character.face_grid.x - 0.01f)) : 0;
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

    sf::FloatRect bounds = text.getGlobalBounds();
    float x = bounds.position.x + bounds.size.x + 26;
    if (player.shielded) {
        canvas.hexagon({x, bounds.getCenter().y}, 12, SHIELD_COLOR, 0);
        x += 34;
    }
    // The Trauma Team on call: a white tag with a red cross, until it comes
    if (player_num == me && trauma_ready) {
        sf::RectangleShape tag({26, 26});
        tag.setOrigin({13, 13});
        tag.setPosition({x, bounds.getCenter().y});
        canvas.draw(tag);
        for (sf::Vector2f size : {sf::Vector2f(18, 6), sf::Vector2f(6, 18)}) {
            sf::RectangleShape bar(size);
            bar.setOrigin(size / 2.f);
            bar.setPosition(tag.getPosition());
            bar.setFillColor(NEON_RED);
            canvas.draw(bar);
        }
    }

}

// Your timed power-ups drain a gauge each, stacked; the Sandevistan also tints the screen magenta
void GameScreen::drawPowerUps(const PlayerState &player, int me)
{
    const PlayerAnim &anim = run.player_anims[me];
    float y = 40;

    if (player.form == Form::Titan) {
        const Character &mount = sprites.mount(run.characterOf(me));
        drawGauge(&mount == &sprites.eren_titan ? "ATTACK TITAN" : mount.name, NEON_ORANGE, 1 - anim.form_clock.getElapsedTime().asSeconds() / titan_time, y);
        y += 80;
    } else if (player.sprinting) {
        sf::RectangleShape tint({Screen::WIDTH, Screen::HEIGHT});
        tint.setFillColor(withAlpha(SANDEVISTAN_COLOR, 22));
        canvas.draw(tint, sf::BlendAdd);
        drawGauge("SANDEVISTAN", SANDEVISTAN_COLOR, 1 - anim.sprint_clock.getElapsedTime().asSeconds() /
                                                       (sandevistan_time * (boon == Boon::Overclock ? 2 : 1)), y);
        y += 80;
    }
    if (player.berserk)
        drawGauge("BERSERK", BERSERK_COLOR, 1 - anim.berserk_clock.getElapsedTime().asSeconds() /
                                                 (BERSERK_TIME * (boon == Boon::Rage ? 2 : 1)), y);
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
    if (phase == Phase::Countdown && countdown > 0) {
        canvas.glitchText(std::to_string(countdown), 220, NEON_YELLOW, Screen::CENTER);
        drawLoadout();
    }
    else if (phase == Phase::Running && phase_clock.getElapsedTime().asSeconds() < 1.2f)
        canvas.glitchText("BURN THE CITY", 120, NEON_RED, Screen::CENTER);
}

// What you bought, under the countdown: the shop's work shows at every start
void GameScreen::drawLoadout()
{
    std::string line;

    for (int i = 0; i < UPGRADE_COUNT; i++) {
        if (upgrades[i] == 0)
            continue;
        if (!line.empty())
            line += "     ";
        line += upgradeLabel(i, upgrades[i]);
    }
    if (!line.empty())
        canvas.text(line, 24, NEON_CYAN, Screen::CENTER + sf::Vector2f(0, 170));
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

    canvas.text(std::format("WALLET  {} EDDIES", wallet), 28, NEON_PINK, Screen::CENTER + sf::Vector2f(0, 150));
    drawNextGoal(Screen::CENTER + sf::Vector2f(0, 192));
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
    drawDeathCause(center + sf::Vector2f(0, -55));
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
    else if (winner >= 0) {
        canvas.glitchText("FLATLINED", 140, NEON_RED, title_pos);
        drawDeathCause(center + sf::Vector2f(0, -55));
    }
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

void GameScreen::drawDeathCause(sf::Vector2f pos)
{
    static const std::map<char, const char *> causes = {
        {ZAPPER_TILE, "FRIED ON A ZAPPER"}, {DRONE_TILE, "RAMMED A DRONE"}, {TITAN_HIT, "RUN DOWN BY A TITAN"},
        {SNIPER_HIT, "SNIPED BY ARASAKA"}, {BOSS_HIT, "SMASHED BY ADAM SMASHER"}};
    auto cause = causes.find(death_cause);

    if (cause != causes.end())
        canvas.text(cause->second, 34, DIM_WHITE, pos);
}

// A reason for one more run: the cheapest thing left in the shop, and how far it is
void GameScreen::drawNextGoal(sf::Vector2f pos)
{
    if (next_goal.price < 0)
        return;
    std::string name = next_goal.runner >= 0 ? sprites.character(next_goal.runner).name
                                             : upgradeLabel(next_goal.upgrade, upgrades[next_goal.upgrade] + 1);
    for (char &c : name)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (wallet >= next_goal.price)
        canvas.text(std::format("NEXT: {}, YOURS IN THE SHOP", name), 26, NEON_GREEN, pos);
    else
        canvas.text(std::format("NEXT: {}, {} EDDIES TO GO", name, next_goal.price - wallet), 26, DIM_WHITE, pos);
}

void GameScreen::drawRematchPrompt(GameState &state, bool solo)
{
    sf::Vector2f pos = Screen::CENTER + sf::Vector2f(0, 250);

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

// A deadly hit with the Trauma Team on call: come or not, the bill shown, the time left draining
void GameScreen::drawTraumaOffer(const PlayerState &me)
{
    canvas.overlay(170);
    sf::Vector2f center = Screen::CENTER;
    canvas.glitchText("FLATLINING", 110, NEON_RED, center + sf::Vector2f(0, -250));
    canvas.text("CALL THE TRAUMA TEAM?", 60, NEON_CYAN, center + sf::Vector2f(0, -120));
    canvas.text(std::format("THEY BILL HALF THE RUN: {} EDDIES", static_cast<int>(me.score * TRAUMA_BILL)), 34,
                sf::Color::White, center + sf::Vector2f(0, -45));

    float left = std::clamp(1 - trauma_offer_clock.getElapsedTime().asSeconds() / TRAUMA_OFFER_TIME, 0.0f, 1.0f);
    sf::RectangleShape bar({600 * left, 10});
    bar.setPosition(center + sf::Vector2f(-300, 30));
    bar.setFillColor(NEON_CYAN);
    canvas.draw(bar);
    canvas.text("THEY COME ON THEIR OWN WHEN IT RUNS OUT", 24, withAlpha(sf::Color::White, 170),
                center + sf::Vector2f(0, 70));

    for (bool call : {true, false}) {
        sf::Vector2f pos = call ? Touch::TRAUMA_CALL : Touch::TRAUMA_DECLINE;
        canvas.card(sf::FloatRect(pos - Touch::TRAUMA_BUTTON_SIZE / 2.f, Touch::TRAUMA_BUTTON_SIZE), call);
        canvas.text(call ? Touch::hint("[Y] CALL THEM", "CALL THEM") : Touch::hint("[N] LET ME DIE", "LET ME DIE"), 40,
                    call ? NEON_CYAN : NEON_RED, pos);
    }
}

// Three cards to pick from, 1 to 3, and walking out (0): the Ripperdoc's implants, the boons. The time
// left drains under the title.
void GameScreen::drawCards(const std::string &title, sf::Color color, const std::string &subtitle, float left,
                           const std::vector<Card> &cards)
{
    canvas.overlay(170);
    sf::Vector2f center = Screen::CENTER;
    canvas.glitchText(title, 110, color, center + sf::Vector2f(0, -330));
    canvas.text(subtitle, 32, sf::Color::White, center + sf::Vector2f(0, -230));

    sf::RectangleShape bar({600 * std::clamp(left, 0.0f, 1.0f), 10});
    bar.setPosition(center + sf::Vector2f(-300, -180));
    bar.setFillColor(color);
    canvas.draw(bar);

    for (size_t i = 0; i < cards.size(); i++) {
        const Card &card = cards[i];
        sf::Vector2f pos = Touch::implantCard(static_cast<int>(i) + 1);
        sf::Color dim = withAlpha(sf::Color::White, 90);
        canvas.card(sf::FloatRect(pos - Touch::IMPLANT_CARD_SIZE / 2.f, Touch::IMPLANT_CARD_SIZE), card.enabled);
        canvas.text(Touch::ENABLED ? card.name : std::format("[{}] {}", i + 1, card.name), 30, card.enabled ? color : dim,
                    pos + sf::Vector2f(0, -60));
        canvas.text(card.what, 22, card.enabled ? sf::Color::White : dim, pos);
        if (!card.price.empty())
            canvas.text(card.price, 32, card.enabled ? NEON_YELLOW : dim, pos + sf::Vector2f(0, 60));
    }
    sf::Vector2f out = Touch::implantCard(0);
    canvas.card(sf::FloatRect(out - sf::Vector2f(210, 55), {420, 110}), false);
    canvas.text(Touch::hint("[0] NO THANKS", "NO THANKS"), 36, NEON_RED, out);
}

// Past Smasher: three implants for the run's eddies, the ones you can't afford dimmed
void GameScreen::drawRipperdoc(const PlayerState &me)
{
    std::vector<Card> cards;
    for (int i = 1; i <= IMPLANTS; i++)
        cards.push_back({IMPLANT_LOOKS[i].name, IMPLANT_LOOKS[i].what, std::format("{} EDDIES", IMPLANT_PRICES[i]),
                         me.score >= IMPLANT_PRICES[i]});
    drawCards("RIPPERDOC", NEON_CYAN, std::format("ONE IMPLANT FOR THE REST OF THE RUN     YOU HAVE {} EDDIES", me.score),
              1 - ripperdoc_clock.getElapsedTime().asSeconds() / RIPPERDOC_TIME, cards);
}

// At BOON_DISTANCE: three boons, free, the pact in red
void GameScreen::drawBoons(int me)
{
    std::vector<Card> cards;
    for (Boon card : boon_cards) {
        BoonLook look = boonLook(card, weaponOf(run.characterOf(me)));
        bool pact = static_cast<int>(card) >= FIRST_PACT && card != Boon::Signature;
        cards.push_back({look.name, look.what, pact ? "RISKY" : card == Boon::Signature ? "YOURS ONLY" : "", true});
    }
    drawCards("PICK A BOON", NEON_YELLOW, "ONE FOR THE REST OF THE RUN, ON THE HOUSE",
              1 - boon_clock.getElapsedTime().asSeconds() / BOON_TIME, cards);
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
