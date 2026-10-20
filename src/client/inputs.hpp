// Keyboard: jetpack, attack, ultimate, confirm and back

#ifndef INPUTS_HPP
    #define INPUTS_HPP

#include <SFML/Window.hpp>
#include "settings.hpp"

class Client;

class InputManager {
private:
    Client *client;
    sf::Window *window;
    sf::Keyboard::Key jetpack_key;
    sf::Keyboard::Key attack_key;
    sf::Keyboard::Key ultimate_key;
    bool jet_active;
    bool attack_held;

    // Set for the frame they happened in
    bool close_requested;
    bool back_requested;
    bool confirm_pressed;

public:
    InputManager(Client *client, sf::Window *window, const Settings &settings);
    void processInputs();

    bool closeRequested() const { return close_requested; }
    bool backRequested() const { return back_requested; }
    bool confirmPressed() const { return confirm_pressed; }
};

#endif
