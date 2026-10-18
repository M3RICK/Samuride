// The districts' scenery: parallax layers cut from big panoramas, looping with crossfaded seams

#ifndef BACKDROPS_HPP
    #define BACKDROPS_HPP

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

class Canvas;
struct WorldView;

class Backdrops {
public:
    struct Segment {
        const sf::Texture *texture;
        sf::IntRect source;
    };

    // A district's look: one image cut into horizontal layers, each scrolling at its own speed.
    // A layer can also chain several pictures one after the other, to avoid looping too soon.
    struct Backdrop {
        struct Layer {
            float top;          // fractions of the main image height
            float bottom;
            float parallax;     // 1 moves with the map
            std::vector<Segment> segments;  // empty: the band of the main image
        };

        std::string name;
        std::string tagline;
        const sf::Texture *texture;     // main image: calibration and layers without segments
        // Calibration knobs: image height fractions landing on the top of the map and on the feet line
        float ceiling;
        float floor;
        sf::Color tint;
        std::vector<Layer> layers;
    };

private:
    // BOTH WORLDS: the districts take turns every ZONE_LENGTH metres, crossfading over ZONE_FADE
    static constexpr float ZONE_LENGTH = 250;
    static constexpr float ZONE_FADE = 25;
    // The pictures don't tile seamlessly, copies crossfade over this share of their width
    static constexpr float OVERLAP = 0.12f;

    std::vector<Backdrop> backdrops;
    std::map<std::string, sf::Texture> textures;   // shared between backdrops, loaded once

    const sf::Texture *sharedTexture(const std::string &path);
    Segment thumbnailOf(const Backdrop &backdrop, size_t index) const;
    void drawLayer(Canvas &canvas, const WorldView &view, const Backdrop &backdrop, const Backdrop::Layer &layer,
                   float y0, float y1, sf::Color color) const;
    void drawFadingPicture(Canvas &canvas, const Segment &segment, sf::FloatRect dest, float fade, sf::Color color) const;

public:
    void load();
    const Backdrop &operator[](size_t index) const { return backdrops[index]; }

    void draw(Canvas &canvas, const WorldView &view, size_t index, float opacity) const;
    // Returns the backdrop mostly on screen: BOTH WORLDS takes turns by distance
    int drawDistrict(Canvas &canvas, const WorldView &view, int district) const;
    // One picture per thumbnail slice, for the district select
    std::vector<Segment> slices(int district) const;
};

#endif
