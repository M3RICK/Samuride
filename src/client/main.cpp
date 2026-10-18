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
static Client::RunResult play(Renderer &renderer, const std::string &host, int port, Loadout loadout,
                              bool debug_mode, bool &connected)
{
    GameState game_state;
    Client client(host, port, loadout, debug_mode, game_state);

    connected = client.initialize();
    if (!connected)
        return Client::RunResult::Menu;

    InputManager input_manager(&client, &renderer.getWindow());
    return client.run(input_manager, renderer);
}

struct Options {
    std::string host;
    int port = -1;
    int character = 0;
    bool debug_mode = false;
};

static bool parseOptions(int argc, char **argv, Options &options)
{
    int opt;

    while ((opt = getopt(argc, argv, "h:p:c:d")) != -1) {
        switch (opt) {
            case 'h':
                options.host = optarg;
                break;
            case 'p':
                options.port = std::atoi(optarg);
                break;
            case 'c':
                options.character = std::atoi(optarg);
                break;
            case 'd':
                options.debug_mode = true;
                break;
            default:
                return false;
        }
    }
    // -h and -p go together
    return options.host.empty() == (options.port <= 0);
}

// Solo and host runs play on a server we start ourselves; joining uses the address typed in
static bool resolveServer(MenuChoice &choice, LocalServer &server)
{
    bool solo = choice.mode == MenuChoice::Mode::Solo;

    if (choice.mode == MenuChoice::Mode::Join)
        return true;

    choice.host = "127.0.0.1";
    choice.port = solo ? 40000 + std::random_device{}() % 20000 : HOST_PORT;
    return server.start(choice.port, solo);
}

static std::string connectionError(const MenuChoice &choice)
{
    if (choice.mode == MenuChoice::Mode::Host)
        return "PORT " + std::to_string(HOST_PORT) + " IS BUSY";
    return "CAN'T REACH " + choice.host + ":" + std::to_string(choice.port);
}

// Menu, run, back to the menu, until the player quits
static void menuLoop(Renderer &renderer, Menu &menu, bool debug_mode)
{
    while (true) {
        MenuChoice choice = menu.run(renderer);
        LocalServer local_server;
        bool connected = true;

        if (choice.mode == MenuChoice::Mode::Quit)
            return;
        if (!resolveServer(choice, local_server)) {
            menu.setError("CAN'T START THE SERVER");
            continue;
        }

        bool hosting = choice.mode == MenuChoice::Mode::Host;
        renderer.setLobbyInfo(hosting ? "YOUR CHOOM JOINS ON  " + lanAddress() + ":" + std::to_string(HOST_PORT) : "");

        Loadout loadout = {choice.character, choice.district, choice.game_mode};
        if (play(renderer, choice.host, choice.port, loadout, debug_mode, connected)
            == Client::RunResult::Quit)
            return;
        if (!connected)
            menu.setError(connectionError(choice));
    }
}

int main(int argc, char **argv)
{
    Options options;

    if (!parseOptions(argc, argv, options)) {
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

    // Direct connection from the command line, then the menu as usual
    if (!options.host.empty()) {
        bool connected = true;
        if (play(renderer, options.host, options.port, {options.character}, options.debug_mode, connected)
            == Client::RunResult::Quit)
            return EXIT_SUCCESS;
        if (!connected)
            menu.setError("CAN'T REACH " + options.host + ":" + std::to_string(options.port));
    }

    menuLoop(renderer, menu, options.debug_mode);
    return EXIT_SUCCESS;
}
