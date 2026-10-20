// Keyboard or touch screen: jetpack, attack, ultimate, confirm and back

#include "inputs.hpp"
#include "client.hpp"
#include "touch.hpp"

InputManager::InputManager(Client *client, sf::RenderWindow *window, const Settings &settings)
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
            if (key->code == attack_key)
                pressAttack();
            if (key->code == ultimate_key)
                client->sendUltimate();
        } else if (const auto *touch = event->getIf<sf::Event::TouchBegan>()) {
            // The screen view the HUD was last drawn with, buttons included
            sf::Vector2f pos = window->mapPixelToCoords(touch->position);
            fingers[touch->finger] = pos;
            confirm_pressed = true;     // a tap is a rematch, once a game is over
            if (Touch::on(Touch::ATTACK, pos))
                pressAttack();
            else if (Touch::on(Touch::ULTIMATE, pos))
                client->sendUltimate();
        } else if (const auto *moved = event->getIf<sf::Event::TouchMoved>()) {
            fingers[moved->finger] = window->mapPixelToCoords(moved->position);
        } else if (const auto *ended = event->getIf<sf::Event::TouchEnded>()) {
            fingers.erase(ended->finger);
        }
    }

    // Focus check: two clients on the same machine must not share the keyboard
    bool thrust = window->hasFocus() && sf::Keyboard::isKeyPressed(jetpack_key);
    bool attack_down = window->hasFocus() && sf::Keyboard::isKeyPressed(attack_key);
    pollTouches(thrust, attack_down);

    if (attack_held && !attack_down) {
        attack_held = false;
        client->sendAttack(false);
    }

    if (thrust != jet_active) {
        jet_active = thrust;
        client->sendPlayerInput(jet_active);
    }
}

// CYBERPSYCHO; the server ignores it in CLASSIC. Key repeats don't count, holding does.
void InputManager::pressAttack()
{
    if (attack_held)
        return;
    attack_held = true;
    client->sendAttack(true);
}

// Fingers down: on the attack button it's held, anywhere off the buttons it's the jetpack
void InputManager::pollTouches(bool &thrust, bool &attack_down) const
{
    for (const auto &[finger, pos] : fingers) {
        if (Touch::on(Touch::ATTACK, pos))
            attack_down = true;
        else if (!Touch::on(Touch::ULTIMATE, pos))
            thrust = true;
    }
}
