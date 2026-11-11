// The finished frame's last coat: the neons bleed light, the colours split on a hit, the corners darken

#ifndef POSTFX_HPP
    #define POSTFX_HPP

#include <SFML/Graphics.hpp>

class PostFx {
private:
    // The glow is blurred at a quarter of the frame's size: wider for the same work, and softer
    static constexpr unsigned int GLOW_SHRINK = 4;
    static constexpr float GLOW_STRENGTH = 0.55f;

    sf::RenderTexture frame;
    sf::RenderTexture scenery;    // the frame as it was with only its backdrop drawn
    bool has_scenery = false;
    sf::RenderTexture glow[2];
    sf::Shader bright;
    sf::Shader blur;
    sf::Shader compose;
    bool ready = false;

    void pass(const sf::Texture &from, sf::RenderTarget &to, const sf::Shader *shader);

public:
    // For a window of this size. False where the graphics card has no shaders (phones): the frame is
    // then drawn straight to the window, as it always was.
    bool open(sf::Vector2u size);
    bool on() const { return ready; }
    // What the frame is drawn on while it is on
    sf::RenderTarget &target() { return frame; }
    // Call once the frame's backdrop is drawn: it won't glow, only what comes over it will
    void sceneryDone();
    // The frame goes to the window. `split` pulls the red and blue apart, as a share of the screen at
    // its edges (0: none).
    void present(sf::RenderTarget &window, float split);
};

#endif
