/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "render.hpp"
#include "menu.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

//=============================================================================
// Camera
//=============================================================================

sf::Vector2f Renderer::toScreen(sf::Vector2f world) const
{
    return {(world.x - camera_x) * tile_size, PLAY_TOP + world.y * tile_size};
}

// The server ticks at 60Hz, easing toward its positions hides the jitter
void Renderer::smoothPositions(const std::map<int, PlayerState> &players, float dt)
{
    for (const auto &[player_num, player] : players) {
        sf::Vector2f target(player.x, player.y);
        auto [it, inserted] = display_positions.try_emplace(player_num, target);
        sf::Vector2f &pos = it->second;

        if ((target - pos).length() > 3)
            pos = target;
        else
            pos += (target - pos) * std::min(1.0f, dt * 20);
    }
}

void Renderer::updateCamera(int me)
{
    auto it = display_positions.find(me);

    if (it != display_positions.end())
        camera_x = std::max(0.0f, it->second.x - SCREEN_WIDTH / tile_size / 4);
}

//=============================================================================
// Backdrops
//=============================================================================

// BOTH WORLDS takes turns by distance, so every player sees the switch at the same spot
void Renderer::renderDistrict(int district)
{
    int shown = std::clamp(district, 0, static_cast<int>(backdrops.size()) - 1);
    int next = shown;
    float fade = 0;

    if (district == MenuLayout::DISTRICT_MIX) {
        int zone = static_cast<int>(camera_x / ZONE_LENGTH);
        float into_zone = camera_x - zone * ZONE_LENGTH;

        shown = zone % 2;
        next = (zone + 1) % 2;
        fade = std::clamp((into_zone - (ZONE_LENGTH - ZONE_FADE)) / ZONE_FADE, 0.0f, 1.0f);
    }

    renderBackdrop(backdrops[shown], 1);
    renderBackdrop(backdrops[next], fade);

    int current = fade > 0.5f ? next : shown;
    if (current != shown_backdrop) {
        shown_backdrop = current;
        banner = "ENTERING " + backdrops[current].name;
        banner_tagline = backdrops[current].tagline;
        banner_clock.restart();
    }
}

// Scaled so the backdrop's ceiling and feet line land on the top and bottom of the map
void Renderer::renderBackdrop(const Backdrop &backdrop, float opacity)
{
    sf::Vector2f size(backdrop.texture->getSize());

    if (size.y == 0 || opacity <= 0)
        return;

    float scale = (PLAY_BOTTOM - PLAY_TOP) / ((backdrop.floor - backdrop.ceiling) * size.y);
    float image_top = PLAY_TOP - backdrop.ceiling * size.y * scale;
    sf::Color color = withAlpha(backdrop.tint, 255 * opacity);

    for (const Backdrop::Layer &layer : backdrop.layers) {
        float y0 = image_top + layer.top * size.y * scale;
        float y1 = image_top + layer.bottom * size.y * scale;
        renderLayer(backdrop, layer, y0, y1, color);
    }
}

// A layer's pictures loop left to right; each keeps its proportions and fills the layer's height
void Renderer::renderLayer(const Backdrop &backdrop, const Backdrop::Layer &layer, float y0, float y1, sf::Color color)
{
    std::vector<Backdrop::Segment> segments = layer.segments;
    sf::Vector2i size(backdrop.texture->getSize());

    if (segments.empty()) {
        int v0 = layer.top * size.y;
        int v1 = layer.bottom * size.y;
        segments.push_back({backdrop.texture, sf::IntRect({0, v0}, {size.x, v1 - v0})});
    }

    std::vector<float> widths;
    for (const Backdrop::Segment &segment : segments)
        widths.push_back(segment.source.size.x * (y1 - y0) / segment.source.size.y);

    // One fade width for every seam, so each picture is fully faded in when the previous one ends
    float fade = BG_OVERLAP * *std::min_element(widths.begin(), widths.end());
    float cycle = 0;
    for (float width : widths)
        cycle += width - fade;

    float x = -std::fmod(camera_x * tile_size * layer.parallax, cycle) - cycle;
    for (size_t i = 0; x < SCREEN_WIDTH; i = (i + 1) % segments.size()) {
        if (x + widths[i] > 0)
            drawFadingPicture(segments[i], sf::FloatRect({x, y0}, {widths[i], y1 - y0}), fade, color);
        x += widths[i] - fade;
    }
}

// The picture fades in over its first `fade` pixels, on top of whatever is already there
void Renderer::drawFadingPicture(const Backdrop::Segment &segment, sf::FloatRect dest, float fade, sf::Color color)
{
    sf::FloatRect src(segment.source);
    float x0 = dest.position.x;
    float x1 = x0 + dest.size.x;
    float y0 = dest.position.y;
    float y1 = y0 + dest.size.y;
    float u0 = src.position.x;
    float u1 = u0 + src.size.x;
    float u_fade = u0 + src.size.x * fade / dest.size.x;
    float v0 = src.position.y;
    float v1 = v0 + src.size.y;
    sf::Color clear = withAlpha(color, 0);
    sf::VertexArray quads(sf::PrimitiveType::Triangles);

    auto addQuad = [&](float left, float right, float ua, float ub, sf::Color left_color) {
        quads.append({{left, y0}, left_color, {ua, v0}});
        quads.append({{right, y0}, color, {ub, v0}});
        quads.append({{right, y1}, color, {ub, v1}});
        quads.append({{left, y0}, left_color, {ua, v0}});
        quads.append({{right, y1}, color, {ub, v1}});
        quads.append({{left, y1}, left_color, {ua, v1}});
    };

    addQuad(x0, x0 + fade, u0, u_fade, clear);
    addQuad(x0 + fade, x1, u_fade, u1, color);
    window.draw(quads, sf::RenderStates(segment.texture));
}

//=============================================================================
// Map Tiles
//=============================================================================

void Renderer::renderMapTiles(const std::set<std::pair<int, int>> &collected)
{
    int start_x = std::max(0, static_cast<int>(camera_x) - 1);
    int end_x = std::min(static_cast<int>(world_map.getWidth()), static_cast<int>(camera_x + SCREEN_WIDTH / tile_size) + 2);
    float time = animation_clock.getElapsedTime().asSeconds();

    for (size_t y = 0; y < world_map.getHeight(); y++) {
        for (int x = start_x; x < end_x; x++) {
            char tile = world_map.getTile(x, y);

            if (tile == 'c' && !collected.count({x, y}))
                renderCoin(x, y, time);
            else if (tile == 'e')
                renderZapper(x, y);
        }
    }
}

// Bobbing coin with a soft pink halo
void Renderer::renderCoin(int x, int y, float time)
{
    sf::Vector2f world(x, y + std::sin(time * 4 + x) * 0.06f);

    drawHalo(toScreen(world + sf::Vector2f(0.5f, 0.5f)), tile_size * 0.32f, withAlpha(NEON_PINK, 40));
    drawTileSprite(coin_texture, world, 0.55f, sf::Color::White);
}

// Jetpack Joyride zapper: neighbouring nodes are linked by a live beam
void Renderer::renderZapper(int x, int y)
{
    static const sf::Vector2i links[] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    sf::Vector2f center = toScreen({x + 0.5f, y + 0.5f});
    bool linked = false;

    for (sf::Vector2i link : links) {
        if (y + link.y >= 0 && world_map.getTile(x + link.x, y + link.y) == 'e') {
            drawBeam(center, toScreen({x + link.x + 0.5f, y + link.y + 0.5f}));
            linked = true;
        }
    }

    // A lone node just crackles
    for (int i = 0; !linked && i < 3; i++) {
        float angle = randf(0, 6.2832f);
        sf::Vector2f tip = center + sf::Vector2f(std::cos(angle), std::sin(angle)) * tile_size * randf(0.4f, 0.7f);
        drawLightning(center, tip, i == 0 ? sf::Color::White : NEON_CYAN);
    }

    float pulse = 0.8f + 0.2f * std::sin(animation_clock.getElapsedTime().asSeconds() * 14 + x);
    drawHalo(center, tile_size * 0.5f * pulse, withAlpha(NEON_CYAN, 70));

    sf::Color flicker(255, 255, 255, std::rand() % 8 ? 255 : 140);
    drawTileSprite(electric_texture, {static_cast<float>(x), static_cast<float>(y)}, 0.55f, flicker);
}

void Renderer::drawBeam(sf::Vector2f from, sf::Vector2f to)
{
    sf::Vector2f delta = to - from;
    float flicker = randf(0.7f, 1);

    sf::RectangleShape glow({delta.length(), tile_size * 0.35f * flicker});
    glow.setOrigin({0, glow.getSize().y / 2});
    glow.setPosition(from);
    glow.setRotation(delta.angle());
    glow.setFillColor(withAlpha(NEON_CYAN, 60));
    window.draw(glow, sf::BlendAdd);

    sf::RectangleShape core({delta.length(), 4});
    core.setOrigin({0, 2});
    core.setPosition(from);
    core.setRotation(delta.angle());
    core.setFillColor(withAlpha(sf::Color::White, 200 * flicker));
    window.draw(core, sf::BlendAdd);

    drawLightning(from, to, NEON_CYAN);
    drawLightning(from, to, sf::Color::White);
}

void Renderer::drawLightning(sf::Vector2f from, sf::Vector2f to, sf::Color color)
{
    const int segments = 6;
    sf::Vector2f normal = (to - from).perpendicular().normalized();
    sf::VertexArray bolt(sf::PrimitiveType::LineStrip);

    for (int i = 0; i <= segments; i++) {
        float jitter = (i == 0 || i == segments) ? 0 : randf(-0.15f, 0.15f) * tile_size;
        bolt.append({from + (to - from) * (i / static_cast<float>(segments)) + normal * jitter, color});
    }
    window.draw(bolt, sf::BlendAdd);

    // Lines are 1px, a second pass makes the arc readable once the window is scaled down
    for (size_t i = 0; i < bolt.getVertexCount(); i++)
        bolt[i].position += {1.5f, 1.5f};
    window.draw(bolt, sf::BlendAdd);
}

// Fits a texture inside the tile at world position, centered
void Renderer::drawTileSprite(const sf::Texture &texture, sf::Vector2f world, float fill, sf::Color color)
{
    sf::Vector2f size(texture.getSize());

    if (size.x == 0)
        return;

    sf::Sprite sprite(texture);
    float scale = tile_size * fill / std::max(size.x, size.y);

    sprite.setOrigin(size / 2.f);
    sprite.setScale({scale, scale});
    sprite.setPosition(toScreen(world + sf::Vector2f(0.5f, 0.5f)));
    sprite.setColor(color);
    window.draw(sprite);
}

//=============================================================================
// Players
//=============================================================================

void Renderer::renderPlayers(const std::map<int, PlayerState> &players, int me)
{
    bool live = last_phase == Phase::Running;

    // Exhaust stays in the world, so it trails behind as the player runs
    for (const auto &[player_num, player] : players) {
        sf::Vector2f nozzle = display_positions[player_num] + sf::Vector2f(0.25f, 1.55f);

        if (live && player.jet_active) {
            emitParticles(nozzle, 2, characterOf(player_num).exhaust, 1.5f, 0.35f, 0, {0, 8});
            emitParticles(nozzle, 1, NEON_ORANGE, 2.5f, 0.25f, 0, {0, 6});
        }
    }

    // Others first so you are always drawn on top
    for (const auto &[player_num, player] : players) {
        if (player_num != me)
            renderPlayerSprite(player_num, player, false);
    }

    auto mine = players.find(me);
    if (mine != players.end())
        renderPlayerSprite(me, mine->second, true);
}

// Returns whether the player is on the ground; leaving it starts the take-off clock
bool Renderer::updateAirState(int player_num)
{
    PlayerAnim &state = player_anims[player_num];
    bool on_ground = display_positions[player_num].y >= world_map.getHeight() - PLAYER_HEIGHT - 0.01f;

    if (on_ground) {
        state.on_ground = true;
    } else if (state.on_ground) {
        state.on_ground = false;
        state.air_clock.restart();
    }
    return on_ground;
}

// Animation follows what the player does: run, take off, rise, hover at the ceiling, fall, die
Renderer::AnimationFrame Renderer::pickAnimation(int player_num, const PlayerState &player, bool on_ground)
{
    const Character &character = characterOf(player_num);
    auto death = death_clocks.find(player_num);
    float air_time = player_anims[player_num].air_clock.getElapsedTime().asSeconds();
    float time = animation_clock.getElapsedTime().asSeconds();

    if (death != death_clocks.end() && character.death.frames > 0)
        return {&character.death, death->second.getElapsedTime().asSeconds()};
    if (on_ground)
        return {&character.run, time};
    if (air_time < character.takeoff.duration())
        return {&character.takeoff, air_time};
    if (!player.jet_active)
        return {&character.fall, time};

    bool at_ceiling = display_positions[player_num].y <= 0.01f;
    return {at_ceiling ? &character.hover : &character.rise, time};
}

void Renderer::renderPlayerSprite(int player_num, const PlayerState &player, bool is_me)
{
    const Character &character = characterOf(player_num);
    AnimationFrame animation = pickAnimation(player_num, player, updateAirState(player_num));
    int frame = animation.animation->frameAt(animation.time);

    if (frame < 0 || character.sheet.getSize().x == 0)
        return;

    // Electrocution: shaking and flashing, for good if there is no death animation
    auto death = death_clocks.find(player_num);
    bool shocked = death != death_clocks.end() &&
                   (character.death.frames == 0 || death->second.getElapsedTime().asSeconds() < 0.4f);
    sf::Vector2f feet = toScreen(display_positions[player_num] + sf::Vector2f(0.5f, PLAYER_HEIGHT));
    float scale = tile_size * PLAYER_HEIGHT * 1.05f / character.body;

    if (shocked)
        feet += {randf(-6, 6), randf(-6, 6)};

    sf::Sprite sprite(character.sheet);
    sprite.setTextureRect(character.frameRect(*animation.animation, frame));
    sprite.setOrigin({character.frame_size.x / 2.f, character.feet});
    sprite.setScale({scale, scale});
    sprite.setPosition(feet);

    if (shocked)
        sprite.setColor(std::rand() % 2 ? NEON_CYAN : sf::Color::White);
    else if (!is_me)
        sprite.setColor(sf::Color(255, 255, 255, 140));
    window.draw(sprite);

    if (shocked && std::rand() % 2)
        window.draw(sprite, sf::BlendAdd);

    sf::Vector2f tag_pos = feet - sf::Vector2f(0, tile_size * (PLAYER_HEIGHT + 0.25f));
    window.draw(makeText(is_me ? "YOU" : "CHOOM", 22, is_me ? NEON_YELLOW : NEON_CYAN, tag_pos));
}
