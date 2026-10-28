// Keyboard or touch screen: jetpack, attack, ultimate, the Trauma Team's question, confirm and back

#include "inputs.hpp"
#include "client.hpp"
#include "touch.hpp"

InputManager::InputManager(Client *client, sf::RenderWindow *window, const Settings &settings)
    : client(client), window(window), jetpack_key(settings.jetpack_key), attack_key(settings.attack_key),
      ultimate_key(settings.ultimate_key),
      jet_active(false), attack_held(false), attack_key_down(false),
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
            if (key->code == attack_key) {
                attack_key_down = true;
                pressAttack();
            }
            if (key->code == ultimate_key)
                client->sendUltimate();
            // The Trauma Team's question: Y (or confirm) calls it, N lets go
            if (key->code == sf::Keyboard::Key::Y || key->code == sf::Keyboard::Key::Enter)
                client->answerTraumaTeam(true);
            if (key->code == sf::Keyboard::Key::N)
                client->answerTraumaTeam(false);
        } else if (const auto *released = event->getIf<sf::Event::KeyReleased>()) {
            if (released->code == attack_key)
                attack_key_down = false;
        } else if (event->is<sf::Event::FocusLost>()) {
            attack_key_down = false;
        } else if (const auto *touch = event->getIf<sf::Event::TouchBegan>()) {
            // The screen view the HUD was last drawn with, buttons included
            sf::Vector2f pos = window->mapPixelToCoords(touch->position);
            fingers[touch->finger] = pos;
            // Once a game is over, the menu button goes back, a tap anywhere else is a rematch
            if (Touch::ENABLED && client->runEnded() && Touch::onMenuButton(pos))
                back_requested = true;
            else
                confirm_pressed = true;
            if (Touch::ENABLED && client->traumaOffered()) {
                if (Touch::onBox(Touch::TRAUMA_CALL, Touch::TRAUMA_BUTTON_SIZE, pos))
                    client->answerTraumaTeam(true);
                else if (Touch::onBox(Touch::TRAUMA_DECLINE, Touch::TRAUMA_BUTTON_SIZE, pos))
                    client->answerTraumaTeam(false);
            }
            if (onButton(Touch::ATTACK, pos))
                pressAttack();
            else if (onButton(Touch::ULTIMATE, pos))
                client->sendUltimate();
        } else if (const auto *moved = event->getIf<sf::Event::TouchMoved>()) {
            fingers[moved->finger] = window->mapPixelToCoords(moved->position);
        } else if (const auto *ended = event->getIf<sf::Event::TouchEnded>()) {
            fingers.erase(ended->finger);
        }
    }

    // Focus check: two clients on the same machine must not share the keyboard
    bool thrust = window->hasFocus() && sf::Keyboard::isKeyPressed(jetpack_key);
    // The attack key from its events: polled, a held key could read as up for a frame, and its key
    // repeats then pressed it again and again
    bool attack_down = window->hasFocus() && attack_key_down;
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
        if (onButton(Touch::ATTACK, pos))
            attack_down = true;
        else if (!onButton(Touch::ULTIMATE, pos))
            thrust = true;
    }
}

// The buttons are only there in CYBERPSYCHO on phones: elsewhere their spot is the jetpack too
bool InputManager::onButton(sf::Vector2f button, sf::Vector2f pos) const
{
    return Touch::ENABLED && client->combatRun() && Touch::on(button, pos);
}
