// Sparks, exhaust and steam: short-lived squares living in the world, in tiles

#ifndef PARTICLES_HPP
    #define PARTICLES_HPP

#include <SFML/Graphics.hpp>
#include <vector>

class Canvas;
struct WorldView;

class Particles {
private:
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life;
        float max_life;
        float size;
        float gravity;
        sf::Color color;
    };

    std::vector<Particle> particles;

public:
    // `count` particles flying out in every direction at up to `speed` tiles/s, plus `base_vel`
    void emit(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
              float gravity, sf::Vector2f base_vel = {0, 0});
    void update(float dt);
    void draw(Canvas &canvas, const WorldView &view) const;
    void clear() { particles.clear(); }
};

#endif
