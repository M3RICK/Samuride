/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "client.hpp"
#include "render.hpp"
#include "inputs.hpp"
#include "state.hpp"
#include "../common/debug.hpp"

#include <iostream>
#include <cstdlib>
#include <unistd.h>

void printUsage(const char *programName)
{
    std::cerr << "Usage: " << programName << " -h <ip> -p <port> [-d]" << std::endl;
    std::cerr << "  -h <ip>     Server IP address" << std::endl;
    std::cerr << "  -p <port>   Server port" << std::endl;
    std::cerr << "  -d          Enable debug mode" << std::endl;
}

int main(int argc, char **argv)
{
    int opt;
    std::string server_ip;
    int server_port = -1;
    bool debug_mode = false;

    while ((opt = getopt(argc, argv, "h:p:d")) != -1) {
        switch (opt) {
            case 'h':
                server_ip = optarg;
                break;
            case 'p':
                server_port = std::atoi(optarg);
                break;
            case 'd':
                debug_mode = true;
                break;
            default:
                printUsage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (server_ip.empty() || server_port <= 0) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    Client client(server_ip, server_port, debug_mode);

    if (!client.initialize()) {
        std::cerr << "Client PROBlEM with init." << std::endl;
        return EXIT_FAILURE;
    }

    Renderer renderer(&client);

    if (!renderer.initialize()) {
        std::cerr << "RENDER PROBLEM init." << std::endl;
        return EXIT_FAILURE;
    }

    InputManager input_manager(&client, &renderer.getWindow());

    GameState game_state;
    client.setGameState(&game_state);

    client.run(input_manager, renderer);

    return EXIT_SUCCESS;
}
