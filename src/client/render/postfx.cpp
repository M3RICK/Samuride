// The finished frame's last coat: the neons bleed light, the colours split on a hit, the corners darken

#include "postfx.hpp"
#include "../../common/debug.hpp"
#include <algorithm>

// What is bright enough to glow, shrunk: four taps so a thin beam doesn't flicker between pixels.
// Saturated colours glow more than white ones: the neons, not a pale wall. With a scenery kept aside,
// only what was drawn over it glows: its lamps and posters would drown the run in haze.
static const char *BRIGHT = R"(
uniform sampler2D source;
uniform sampler2D scenery;
uniform float masked;
uniform vec2 texel;

vec3 lit(vec2 uv)
{
    vec3 color = texture2D(source, uv).rgb;
    float peak = max(color.r, max(color.g, color.b));
    float low = min(color.r, min(color.g, color.b));
    float vivid = (peak - low) / max(peak, 0.001);
    vec3 change = abs(color - texture2D(scenery, uv).rgb);
    float fresh = max(1.0 - masked, step(0.03, max(change.r, max(change.g, change.b))));
    return color * smoothstep(0.55, 0.95, peak) * mix(0.35, 1.0, vivid) * fresh;
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    gl_FragColor = vec4((lit(uv + texel * vec2(-1.0, -1.0)) + lit(uv + texel * vec2(1.0, -1.0)) +
                         lit(uv + texel * vec2(-1.0, 1.0)) + lit(uv + texel * vec2(1.0, 1.0))) / 4.0, 1.0);
}
)";

// One direction of a gaussian blur, 9 pixels wide in 5 taps (the card blends between two pixels)
static const char *BLUR = R"(
uniform sampler2D source;
uniform vec2 step;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec3 color = texture2D(source, uv).rgb * 0.227027;
    color += (texture2D(source, uv + step * 1.384615).rgb + texture2D(source, uv - step * 1.384615).rgb) * 0.316216;
    color += (texture2D(source, uv + step * 3.230769).rgb + texture2D(source, uv - step * 3.230769).rgb) * 0.070270;
    gl_FragColor = vec4(color, 1.0);
}
)";

// The frame with its red and blue pulled apart from the centre, plus the glow, darker in the corners
static const char *COMPOSE = R"(
uniform sampler2D source;
uniform sampler2D glow;
uniform float split;
uniform float strength;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec2 pull = (uv - 0.5) * split;
    vec3 color = vec3(texture2D(source, uv + pull).r, texture2D(source, uv).g, texture2D(source, uv - pull).b);
    color += texture2D(glow, uv).rgb * strength;
    color *= 1.0 - 0.3 * smoothstep(0.45, 0.85, distance(uv, vec2(0.5)));
    gl_FragColor = vec4(color, 1.0);
}
)";

bool PostFx::open(sf::Vector2u size)
{
    sf::Vector2u small(std::max(1u, size.x / GLOW_SHRINK), std::max(1u, size.y / GLOW_SHRINK));

    ready = sf::Shader::isAvailable() &&
            bright.loadFromMemory(BRIGHT, sf::Shader::Type::Fragment) &&
            blur.loadFromMemory(BLUR, sf::Shader::Type::Fragment) &&
            compose.loadFromMemory(COMPOSE, sf::Shader::Type::Fragment) &&
            frame.resize(size) && scenery.resize(size) && glow[0].resize(small) && glow[1].resize(small);
    if (!ready) {
        DEBUG_LOG("No shaders here: the frame is drawn without its glow");
        return false;
    }
    frame.setSmooth(true);
    glow[0].setSmooth(true);
    glow[1].setSmooth(true);
    bright.setUniform("texel", sf::Vector2f(1.f / size.x, 1.f / size.y));
    bright.setUniform("scenery", scenery.getTexture());
    compose.setUniform("glow", glow[0].getTexture());
    compose.setUniform("strength", GLOW_STRENGTH);
    return true;
}

// `from` stretched over the whole of `to`, through the shader
void PostFx::pass(const sf::Texture &from, sf::RenderTarget &to, const sf::Shader *shader)
{
    sf::Sprite sprite(from);
    sf::Vector2f from_size(from.getSize());
    sf::Vector2f to_size(to.getSize());
    sf::RenderStates states(shader);

    sprite.setScale({to_size.x / from_size.x, to_size.y / from_size.y});
    states.blendMode = sf::BlendNone;
    to.setView(sf::View(sf::FloatRect({0, 0}, to_size)));
    to.draw(sprite, states);
}

void PostFx::sceneryDone()
{
    frame.display();
    pass(frame.getTexture(), scenery, nullptr);
    scenery.display();
    has_scenery = true;
}

void PostFx::present(sf::RenderTarget &window, float split)
{
    sf::Vector2f small(glow[0].getSize());

    frame.display();
    bright.setUniform("masked", has_scenery ? 1.f : 0.f);
    has_scenery = false;
    pass(frame.getTexture(), glow[0], &bright);
    glow[0].display();
    // Twice, the second one wider: a tight core around each neon and a soft haze further out
    for (float reach : {1.0f, 2.5f}) {
        blur.setUniform("step", sf::Vector2f(reach / small.x, 0));
        pass(glow[0].getTexture(), glow[1], &blur);
        glow[1].display();
        blur.setUniform("step", sf::Vector2f(0, reach / small.y));
        pass(glow[1].getTexture(), glow[0], &blur);
        glow[0].display();
    }
    compose.setUniform("split", split);
    pass(frame.getTexture(), window, &compose);
}
