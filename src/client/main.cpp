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
#include "menu.hpp"

#include <iostream>
#include <cstdlib>
#include <csignal>
#include <filesystem>
#include <random>
#include <string>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

static constexpr int HOST_PORT = 4242;

void printUsage(const char *programName)
{
    std::cerr << "Usage: " << programName << " [-h <ip> -p <port> [-c <character>]] [-d]" << std::endl;
    std::cerr << "  -h <ip>     Server address, skips the menu" << std::endl;
    std::cerr << "  -p <port>   Server port" << std::endl;
    std::cerr << "  -c <n>      Character: 0 Johnny, 1 David, 2 Jinx, 3 Mikasa" << std::endl;
    std::cerr << "  -d          Enable debug mode" << std::endl;
}

// Assets live next to the binary, so the client can be launched from anywhere
static void moveToBinaryDirectory()
{
    std::error_code ec;
    auto dir = std::filesystem::canonical("/proc/self/exe", ec).parent_path();

    if (!ec)
        std::filesystem::current_path(dir, ec);
}

// The jetpack_server sitting next to us, run in the background for solo and host modes
class LocalServer {
private:
    pid_t pid = -1;

public:
    bool start(int port, bool solo)
    {
        // Built before fork: the child must not allocate, SFML threads may hold the malloc lock
        std::string port_arg = std::to_string(port);

        pid = fork();
        if (pid != 0)
            return pid > 0;

        // Dies with the client, even if the client crashes
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);

        execl("./jetpack_server", "jetpack_server", "-p", port_arg.c_str(), solo ? "-s" : nullptr, nullptr);
        _exit(1);
    }

    ~LocalServer()
    {
        if (pid > 0) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

// First non-loopback IPv4, what a friend on the same network should type
static std::string lanAddress()
{
    ifaddrs *list = nullptr;
    std::string address = "127.0.0.1";

    if (getifaddrs(&list) != 0)
        return address;

    for (ifaddrs *it = list; it; it = it->ifa_next) {
        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET)
            continue;
        char buffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(it->ifa_addr)->sin_addr, buffer, sizeof(buffer));
        if (std::string(buffer) != "127.0.0.1") {
            address = buffer;
            break;
        }
    }
    freeifaddrs(list);
    return address;
}

// Plays one session against a server, until the player leaves
static Client::RunResult play(Renderer &renderer, const std::string &host, int port, int character, int district,
                              bool debug_mode, bool &connected)
{
    GameState game_state;
    Client client(host, port, character, district, debug_mode, game_state);

    connected = client.initialize();
    if (!connected)
        return Client::RunResult::Menu;

    InputManager input_manager(&client, &renderer.getWindow());
    return client.run(input_manager, renderer);
}

int main(int argc, char **argv)
{
    int opt;
    std::string server_ip;
    int server_port = -1;
    int character = 0;
    bool debug_mode = false;

    while ((opt = getopt(argc, argv, "h:p:c:d")) != -1) {
        switch (opt) {
            case 'h':
                server_ip = optarg;
                break;
            case 'p':
                server_port = std::atoi(optarg);
                break;
            case 'c':
                character = std::atoi(optarg);
                break;
            case 'd':
                debug_mode = true;
                break;
            default:
                printUsage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (server_ip.empty() != (server_port <= 0)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    moveToBinaryDirectory();

    Renderer renderer;
    if (!renderer.initialize()) {
        std::cerr << "Cannot open game window." << std::endl;
        return EXIT_FAILURE;
    }

    Menu menu(renderer.getWindow());
    bool connected = true;

    // Direct connection from the command line, then the menu as usual
    if (!server_ip.empty()) {
        if (play(renderer, server_ip, server_port, character, -1, debug_mode, connected) == Client::RunResult::Quit)
            return EXIT_SUCCESS;
        if (!connected)
            menu.setError("CAN'T REACH " + server_ip + ":" + std::to_string(server_port));
    }

    while (true) {
        MenuChoice choice = menu.run(renderer);
        LocalServer local_server;
        std::string host = choice.host;
        int port = choice.port;

        if (choice.mode == MenuChoice::Mode::Quit)
            break;

        if (choice.mode == MenuChoice::Mode::Solo) {
            host = "127.0.0.1";
            port = 40000 + std::random_device{}() % 20000;
        } else if (choice.mode == MenuChoice::Mode::Host) {
            host = "127.0.0.1";
            port = HOST_PORT;
        }

        if (choice.mode != MenuChoice::Mode::Join && !local_server.start(port, choice.mode == MenuChoice::Mode::Solo)) {
            menu.setError("CAN'T START THE SERVER");
            continue;
        }

        renderer.setLobbyInfo(choice.mode == MenuChoice::Mode::Host
            ? "YOUR CHOOM JOINS ON  " + lanAddress() + ":" + std::to_string(HOST_PORT) : "");

        if (play(renderer, host, port, choice.character, choice.district, debug_mode, connected) == Client::RunResult::Quit)
            break;
        if (!connected)
            menu.setError(choice.mode == MenuChoice::Mode::Host
                ? "PORT " + std::to_string(HOST_PORT) + " IS BUSY" : "CAN'T REACH " + host + ":" + std::to_string(port));
    }

    return EXIT_SUCCESS;
}
