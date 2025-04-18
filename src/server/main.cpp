/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include "../common/debug.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

void printUsage(const char  *programme)
{
    std::cerr << "Usage: " << programme << " -p <port> -m <map> [-d]" << std::endl;
    std::cerr << "  -p <port>   Port to listen on" << std::endl;
    std::cerr << "  -m <map>    Path to map file" << std::endl;
    std::cerr << "  -d          Enable debug mode" << std::endl;
}

int main(int argc, char** argv)
{
    int port = -1;
    int opt;
    std::string map_path;
    bool debug_mode = false;


    while ((opt = getopt(argc, argv, "p:m:d")) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 'm':
                map_path = optarg;
                break;
            case 'd':
                debug_mode = true;
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    if (port <= 0 || map_path.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    Server server(port, map_path, debug_mode);

    if (!server.initialize()) {
        std::cerr << "Something aint right with the server." << std::endl;
        return 1;
    }

    server.run();

    return 0;
}
