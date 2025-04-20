/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef INPUTS_HPP
    #define INPUTS_HPP

#include <SFML/Window.hpp>
#include <atomic>

class Client;

class InputManager {
private:
    Client *client;
    sf::Window *window;
    std::atomic<bool> jet_active;
    std::atomic<bool> exit_requested;

public:

    InputManager(Client* client, sf::Window *window);
    void processInputs();


    bool isJetActive() const
    {
        return jet_active;
    }

    bool shouldExit() const
    {
        return exit_requested;
    }
};

#endif
