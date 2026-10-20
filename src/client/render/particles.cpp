// Sparks, exhaust and steam: short-lived squares living in the world, in tiles

#include "particles.hpp"
#include "canvas.hpp"
#include <cmath>

void Particles::emit(sf::Vector2f pos, int count, sf::Color color, float speed, float life,
                     float gravity, sf::Vector2f base_vel)
{
    for (int i = 0; i < count && particles.size() < MAX_PARTICLES; i++) {
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

// All in a single draw call, two triangles per square: one call per particle made phones stutter
// during explosions
void Particles::draw(Canvas &canvas, const WorldView &view) const
{
    vertices.resize(particles.size() * 6);

    for (size_t i = 0; i < particles.size(); i++) {
        const Particle &p = particles[i];
        sf::Vector2f center = view.toScreen(p.pos);
        float half = p.size / 2;
        sf::Color color = withAlpha(p.color, 255 * p.life / p.max_life);
        sf::Vector2f corners[4] = {center + sf::Vector2f(-half, -half), center + sf::Vector2f(half, -half),
                                   center + sf::Vector2f(half, half), center + sf::Vector2f(-half, half)};
        int order[6] = {0, 1, 2, 0, 2, 3};

        for (int k = 0; k < 6; k++)
            vertices[i * 6 + k] = sf::Vertex{corners[order[k]], color};
    }
    canvas.draw(vertices, sf::BlendAdd);
}
