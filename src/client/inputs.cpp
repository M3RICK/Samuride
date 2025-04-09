#include "inputs.hpp"
#include "client.hpp"
#include <SFML/Window/Event.hpp>

InputManager::InputManager(Client* client, sf::Window* window)
    : client(client), window(window), jet_active(false), exit_requested(false) {
}

void InputManager::processInputs()
{
    sf::Event event;
    while (window->pollEvent(event)) {
        // Close window event
        if (event.type == sf::Event::Closed) {
            exit_requested = true;
        }

        // Key press events
        if (event.type == sf::Event::KeyPressed) {
            // Exit on Escape
            if (event.key.code == sf::Keyboard::Escape) {
                exit_requested = true;
            }
        }
    }

    // Check for space key (jetpack)
    bool space_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

    // Only send input if the state changed
    if (space_pressed != jet_active) {
        jet_active = space_pressed;

        // Send to server
        client->sendPlayerInput(jet_active);
    }
}
