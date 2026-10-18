/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef INPUTS_HPP
    #define INPUTS_HPP

#include <SFML/Window.hpp>

class Client;

class InputManager {
private:
    Client *client;
    sf::Window *window;
    bool jet_active;

    // Set for the frame they happened in
    bool close_requested;
    bool back_requested;
    bool confirm_pressed;

public:
    InputManager(Client *client, sf::Window *window);
    void processInputs();

    bool closeRequested() const { return close_requested; }
    bool backRequested() const { return back_requested; }
    bool confirmPressed() const { return confirm_pressed; }
};

#endif
