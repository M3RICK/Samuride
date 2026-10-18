/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "inputs.hpp"
#include "client.hpp"

InputManager::InputManager(Client *client, sf::Window *window)
    : client(client), window(window), jet_active(false),
      close_requested(false), back_requested(false), confirm_pressed(false) {
}

void InputManager::processInputs()
{
    back_requested = false;
    confirm_pressed = false;

    while (const std::optional event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>())
            close_requested = true;
        else if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape)
                back_requested = true;
            if (key->code == sf::Keyboard::Key::Space || key->code == sf::Keyboard::Key::Enter)
                confirm_pressed = true;
            // CYBERPSYCHO; the server ignores it in CLASSIC
            if (key->code == sf::Keyboard::Key::F)
                client->sendAttack();
        }
    }

    // Focus check: two clients on the same machine must not share the space bar
    bool space_pressed = window->hasFocus() && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    if (space_pressed != jet_active) {
        jet_active = space_pressed;
        client->sendPlayerInput(jet_active);
    }
}
