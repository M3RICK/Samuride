/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

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

        if (event.type == sf::Event::Closed)
            exit_requested = true;
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape)
                exit_requested = true;
        }
    }

    bool space_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

    if (space_pressed != jet_active) {
        jet_active = space_pressed;
        client->sendPlayerInput(jet_active);
    }
}
