// Keyboard: jetpack, attack, ultimate, confirm and back

#include "inputs.hpp"
#include "client.hpp"

InputManager::InputManager(Client *client, sf::Window *window, const Settings &settings)
    : client(client), window(window), jetpack_key(settings.jetpack_key), attack_key(settings.attack_key),
      ultimate_key(settings.ultimate_key),
      jet_active(false), attack_held(false),
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
            // CYBERPSYCHO; the server ignores it in CLASSIC. Key repeats don't count, holding does.
            if (key->code == attack_key && !attack_held) {
                attack_held = true;
                client->sendAttack(true);
            }
            if (key->code == ultimate_key)
                client->sendUltimate();
        }
    }

    // Focus check: two clients on the same machine must not share the keyboard
    bool thrust = window->hasFocus() && sf::Keyboard::isKeyPressed(jetpack_key);

    if (attack_held && !(window->hasFocus() && sf::Keyboard::isKeyPressed(attack_key))) {
        attack_held = false;
        client->sendAttack(false);
    }

    if (thrust != jet_active) {
        jet_active = thrust;
        client->sendPlayerInput(jet_active);
    }
}
