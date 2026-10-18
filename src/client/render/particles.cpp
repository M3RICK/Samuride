// Sparks, exhaust and steam: short-lived squares living in the world, in tiles

#include "particles.hpp"
#include "canvas.hpp"
#include <cmath>

void Particles::emit(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
                     float gravity, sf::Vector2f base_vel)
{
    for (int i = 0; i < count; i++) {
        float angle = randf(0, 6.2832f);
        sf::Vector2f vel = base_vel + sf::Vector2f(std::cos(angle), std::sin(angle)) * randf(0.2f, 1) * speed;
        float particle_life = life * randf(0.6f, 1);

        particles.push_back({pos, vel, particle_life, particle_life, randf(4, 9), gravity, color});
    }
}

void Particles::update(float dt)
{
    for (Particle &p : particles) {
        p.vel.y += p.gravity * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
    }
    std::erase_if(particles, [](const Particle &p) { return p.life <= 0; });
}

void Particles::draw(Canvas &canvas, const WorldView &view) const
{
    sf::RectangleShape shape;

    for (const Particle &p : particles) {
        shape.setSize({p.size, p.size});
        shape.setOrigin({p.size / 2, p.size / 2});
        shape.setPosition(view.toScreen(p.pos));
        shape.setFillColor(withAlpha(p.color, 255 * p.life / p.max_life));
        canvas.draw(shape, sf::BlendAdd);
    }
}
