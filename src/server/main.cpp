/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "server.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

void printUsage(const char  *programme)
{
    std::cerr << "Usage: " << programme << " -p <port> [-m <map>] [-s] [-d]" << std::endl;
    std::cerr << "  -p <port>   Port to listen on" << std::endl;
    std::cerr << "  -m <map>    Start the run on this map, then keep going procedurally" << std::endl;
    std::cerr << "  -s          Solo run (one player, localhost only)" << std::endl;
    std::cerr << "  -d          Enable debug mode" << std::endl;
}

int main(int argc, char** argv)
{
    int port = -1;
    int opt;
    std::string map_path;
    bool solo = false;
    bool debug_mode = false;

    while ((opt = getopt(argc, argv, "p:m:sd")) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 'm':
                map_path = optarg;
                break;
            case 's':
                solo = true;
                break;
            case 'd':
                debug_mode = true;
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    if (port <= 0) {
        printUsage(argv[0]);
        return 1;
    }

    Server server(port, map_path, solo, debug_mode);

    if (!server.initialize())
        return 1;

    server.run();

    return 0;
}
