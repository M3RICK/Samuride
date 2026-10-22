// Dedicated server entry point: Samuride_server -p <port> [-m <map>] [-s] [-d], or -t for the tuning file

#include "server.hpp"
#include "../common/tuning.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

void printUsage(const char *programme)
{
    std::cerr << "Usage: " << programme << " -p <port> [-m <map>] [-s] [-d]" << std::endl;
    std::cerr << "       " << programme << " -t" << std::endl;
    std::cerr << "  -p <port>   Port to listen on" << std::endl;
    std::cerr << "  -m <map>    Start the run on this map, then keep going procedurally" << std::endl;
    std::cerr << "  -s          Solo run (one player, localhost only)" << std::endl;
    std::cerr << "  -d          Enable debug mode" << std::endl;
    std::cerr << "  -t          Print the tuning file with every value at its default (assets/tuning.txt)" << std::endl;
}

static bool parseOptions(int argc, char **argv, ServerConfig &options)
{
    int opt;

    while ((opt = getopt(argc, argv, "p:m:sdt")) != -1) {
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
            case 't':
                std::cout << Tuning::defaults();
                std::exit(0);
            default:
                return false;
        }
    }
    return options.port > 0;
}

int main(int argc, char **argv)
{
    ServerConfig options;

    if (!parseOptions(argc, argv, options)) {
        printUsage(argv[0]);
        return 1;
    }

    Tuning::load("assets/tuning.txt");
    Server server(options);

    if (!server.initialize())
        return 1;

    server.run();
    return 0;
}
