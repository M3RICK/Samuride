// The districts' scenery: parallax layers cut from big panoramas, looping with crossfaded seams

#include "backdrops.hpp"
#include "canvas.hpp"
#include "../menu.hpp"
#include <algorithm>
#include <cmath>

// The corridor scrolls as one layer; the others run on the Worlds Collide walkway, a panorama far behind
void Backdrops::load()
{
    const sf::Texture *corridor = sharedTexture("assets/background/Resized.png");
    const sf::Texture *walkway = sharedTexture("assets/background/worlds_collide.png");
    const sf::Texture *citadel = sharedTexture("assets/background/neon_citadel.png");

    // neon_citadel.png stacks three panoramas, each cut just above its own (blurry) walkway
    int citadel_width = citadel->getSize().x;
    Segment night_city = {citadel, sf::IntRect({0, 0}, {citadel_width, 260})};
    Segment walls = {citadel, sf::IntRect({0, 283}, {citadel_width, 209})};
    Segment zaun = {citadel, sf::IntRect({0, 513}, {citadel_width, 180})};

    auto onWalkway = [&](const std::string &name, const std::string &tagline,
                         std::vector<Segment> far, float parallax) {
        // Dimmed: the neons would swallow the coins, zappers and drones
        return Backdrop{name, tagline, walkway, 0.05f, 0.825f, sf::Color(165, 160, 185),
                        {{0, 0.8f, parallax, far}, {0.8f, 1, 1, {}}}};
    };

    backdrops = {
        {"THE CORRIDOR", "Arasaka sublevel 3", corridor, 0.20f, 0.79f, sf::Color(215, 215, 215), {{0, 1, 0.4f, {}}}},
        // The three panoramas back to back, a bit faster so a run goes through them
        onWalkway("WORLDS COLLIDE", "Night City, the Walls & Zaun", {night_city, walls, zaun}, 0.25f),
        onWalkway("NIGHT CITY", "Neon towers at sunset", {night_city}, 0.2f),
        onWalkway("THE WALLS", "Dusk along the Walls", {walls}, 0.2f),
        onWalkway("ZAUN", "The undercity never sleeps", {zaun}, 0.2f),
    };
}

const sf::Texture *Backdrops::sharedTexture(const std::string &path)
{
    auto [it, inserted] = textures.try_emplace(path);

    if (inserted)
        loadTexture(it->second, path);
    return &it->second;
}

//=============================================================================
// Drawing
//=============================================================================

int Backdrops::drawDistrict(Canvas &canvas, const WorldView &view, int district) const
{
    int shown = std::clamp(district, 0, static_cast<int>(backdrops.size()) - 1);
    int next = shown;
    float fade = 0;

    if (district == MenuLayout::DISTRICT_MIX) {
        int zone = static_cast<int>(view.camera_x / ZONE_LENGTH);
        float into_zone = view.camera_x - zone * ZONE_LENGTH;

        shown = zone % 2;
        next = (zone + 1) % 2;
        fade = std::clamp((into_zone - (ZONE_LENGTH - ZONE_FADE)) / ZONE_FADE, 0.0f, 1.0f);
    }

    draw(canvas, view, shown, 1);
    draw(canvas, view, next, fade);
    return fade > 0.5f ? next : shown;
}

// Scaled so the backdrop's ceiling and feet line land on the top and bottom of the map
void Backdrops::draw(Canvas &canvas, const WorldView &view, size_t index, float opacity) const
{
    const Backdrop &backdrop = backdrops[index];
    sf::Vector2f size(backdrop.texture->getSize());

    if (size.y == 0 || opacity <= 0)
        return;

    float scale = (Screen::PLAY_BOTTOM - Screen::PLAY_TOP) / ((backdrop.floor - backdrop.ceiling) * size.y);
    float image_top = Screen::PLAY_TOP - backdrop.ceiling * size.y * scale;
    sf::Color color = withAlpha(backdrop.tint, 255 * opacity);

    for (const Backdrop::Layer &layer : backdrop.layers) {
        float y0 = image_top + layer.top * size.y * scale;
        float y1 = image_top + layer.bottom * size.y * scale;
        drawLayer(canvas, view, backdrop, layer, y0, y1, color);
    }
}

// A layer's pictures loop left to right; each keeps its proportions and fills the layer's height
void Backdrops::drawLayer(Canvas &canvas, const WorldView &view, const Backdrop &backdrop,
                          const Backdrop::Layer &layer, float y0, float y1, sf::Color color) const
{
    std::vector<Segment> segments = layer.segments;
    sf::Vector2i size(backdrop.texture->getSize());

    if (segments.empty()) {
        int v0 = layer.top * size.y;
        int v1 = layer.bottom * size.y;
        segments.push_back({backdrop.texture, sf::IntRect({0, v0}, {size.x, v1 - v0})});
    }

    std::vector<float> widths;
    for (const Segment &segment : segments)
        widths.push_back(segment.source.size.x * (y1 - y0) / segment.source.size.y);

    // One fade width for every seam, so each picture is fully faded in when the previous one ends
    float fade = OVERLAP * *std::min_element(widths.begin(), widths.end());
    float cycle = 0;
    for (float width : widths)
        cycle += width - fade;

    float x = -std::fmod(view.camera_x * view.tile_size * layer.parallax, cycle) - cycle;
    for (size_t i = 0; x < Screen::WIDTH; i = (i + 1) % segments.size()) {
        if (x + widths[i] > 0)
            drawFadingPicture(canvas, segments[i], sf::FloatRect({x, y0}, {widths[i], y1 - y0}), fade, color);
        x += widths[i] - fade;
    }
}

// The picture fades in over its first `fade` pixels, on top of whatever is already there
void Backdrops::drawFadingPicture(Canvas &canvas, const Segment &segment, sf::FloatRect dest, float fade,
                                  sf::Color color) const
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
    canvas.draw(quads, sf::RenderStates(segment.texture));
}

//=============================================================================
// Thumbnails
//=============================================================================

// Worlds Collide shows its three panoramas, Both Worlds its two districts
std::vector<Backdrops::Segment> Backdrops::slices(int district) const
{
    if (district == MenuLayout::DISTRICT_MIX)
        return {thumbnailOf(backdrops[0], 0), thumbnailOf(backdrops[1], 0)};

    const Backdrop &backdrop = backdrops[district];
    std::vector<Segment> result;
    size_t count = std::max<size_t>(1, backdrop.layers.front().segments.size());

    for (size_t k = 0; k < count; k++)
        result.push_back(thumbnailOf(backdrop, k));
    return result;
}

// Picture n of a backdrop's far layer, or its play band when the layer is a single image
Backdrops::Segment Backdrops::thumbnailOf(const Backdrop &backdrop, size_t index) const
{
    const std::vector<Segment> &segments = backdrop.layers.front().segments;
    sf::Vector2i size(backdrop.texture->getSize());

    if (!segments.empty())
        return segments[std::min(index, segments.size() - 1)];
    return {backdrop.texture, sf::IntRect({0, static_cast<int>(backdrop.ceiling * size.y)},
                                          {size.x, static_cast<int>((backdrop.floor - backdrop.ceiling) * size.y)})};
}
