// Keyboard or touch screen: jetpack, attack, ultimate, confirm and back

#ifndef INPUTS_HPP
    #define INPUTS_HPP

#include <SFML/Graphics/RenderWindow.hpp>
#include <map>
#include "settings.hpp"

class Client;

class InputManager {
private:
    Client *client;
    sf::RenderWindow *window;
    sf::Keyboard::Key jetpack_key;
    sf::Keyboard::Key attack_key;
    sf::Keyboard::Key ultimate_key;
    bool jet_active;
    bool attack_held;
    bool attack_key_down;   // from its press and release events, so a held key never flickers
    std::map<unsigned int, sf::Vector2f> fingers;   // on the screen, in screen coordinates

    // Set for the frame they happened in
    bool close_requested;
    bool back_requested;
    bool confirm_pressed;

public:
    InputManager(Client *client, sf::RenderWindow *window, const Settings &settings);
    void processInputs();

private:
    void pressAttack();
    void pollTouches(bool &thrust, bool &attack_down) const;
    bool onButton(sf::Vector2f button, sf::Vector2f pos) const;

public:

    bool closeRequested() const { return close_requested; }
    bool backRequested() const { return back_requested; }
    bool confirmPressed() const { return confirm_pressed; }
};

#endif
