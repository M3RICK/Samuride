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

struct Options {
    int port = -1;
    std::string map_path;
    bool solo = false;
    bool debug_mode = false;
};

static bool parseOptions(int argc, char **argv, Options &options)
{
    int opt;

    while ((opt = getopt(argc, argv, "p:m:sd")) != -1) {
        switch (opt) {
            case 'p':
                options.port = std::atoi(optarg);
                break;
            case 'm':
                options.map_path = optarg;
                break;
            case 's':
                options.solo = true;
                break;
            case 'd':
                options.debug_mode = true;
                break;
            default:
                return false;
        }
    }
    return options.port > 0;
}

int main(int argc, char **argv)
{
    Options options;

    if (!parseOptions(argc, argv, options)) {
        printUsage(argv[0]);
        return 1;
    }

    Server server(options.port, options.map_path, options.solo, options.debug_mode);

    if (!server.initialize())
        return 1;

    server.run();
    return 0;
}
