// Game entry point: menu, then runs on a local or remote server, until the player quits

#include "client.hpp"
#include "render/renderer.hpp"
#include "inputs.hpp"
#include "state.hpp"
#include "menu.hpp"

#include "../common/platform.hpp"
#include "../common/socket.hpp"
#include "../server/server.hpp"

#include <iostream>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <thread>
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

// Assets live next to the binary, or in Contents/Resources inside a macOS Samuride.app,
// so the client can be launched from anywhere. On Android SFML reads them out of the APK.
static void moveToAssetsDirectory()
{
#ifndef __ANDROID__
    std::filesystem::path dir = Platform::executableDirectory();
    std::error_code ec;

    if (!std::filesystem::exists(dir / "assets") && std::filesystem::exists(dir / "../Resources/assets"))
        dir /= "../Resources";
    std::filesystem::current_path(dir, ec);
#endif
}

// The server for solo and host runs, in a thread of the game: it stops when the game leaves the run
class LocalServer {
private:
    std::unique_ptr<Server> server;
    std::thread thread;

public:
    bool start(int port, bool solo, bool debug_mode)
    {
        server = std::make_unique<Server>(ServerConfig{port, "", solo, debug_mode, true});
        if (!server->initialize())
            return false;
        thread = std::thread(&Server::run, server.get());
        return true;
    }

    ~LocalServer()
    {
        if (server)
            server->stop();
        if (thread.joinable())
            thread.join();
    }
};

// Plays one session against a server, until the player leaves
static Client::RunResult play(Renderer &renderer, const std::string &host, int port, Loadout loadout,
                              const Settings &settings, bool debug_mode, bool &connected)
{
    GameState game_state;
    Client client(host, port, loadout, debug_mode, game_state);

    connected = client.initialize();
    if (!connected)
        return Client::RunResult::Menu;

    InputManager input_manager(&client, &renderer.getWindow(), settings);
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
static bool resolveServer(MenuChoice &choice, LocalServer &server, bool debug_mode)
{
    bool solo = choice.mode == MenuChoice::Mode::Solo;

    if (choice.mode == MenuChoice::Mode::Join)
        return true;

    choice.host = "127.0.0.1";
    choice.port = solo ? 40000 + std::random_device{}() % 20000 : HOST_PORT;
    return server.start(choice.port, solo, debug_mode);
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
        if (!resolveServer(choice, local_server, debug_mode)) {
            menu.setError("CAN'T START THE SERVER");
            continue;
        }

        bool hosting = choice.mode == MenuChoice::Mode::Host;
        renderer.setLobbyInfo(hosting ? "YOUR CHOOM JOINS ON  " + Socket::lanAddress() + ":" + std::to_string(HOST_PORT) : "");

        Loadout loadout = {choice.character, choice.district, choice.game_mode};
        if (play(renderer, choice.host, choice.port, loadout, menu.getSettings(), debug_mode, connected)
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

    moveToAssetsDirectory();

    Renderer renderer;
    if (!renderer.initialize()) {
        std::cerr << "Cannot open game window." << std::endl;
        return EXIT_FAILURE;
    }

    Settings settings = Settings::load();
    renderer.getAudio().setVolumes(settings.music_volume, settings.sfx_volume);
    if (settings.fullscreen)
        renderer.setFullscreen(true);
    Menu menu(renderer.getWindow(), settings);

    // Direct connection from the command line, then the menu as usual
    if (!options.host.empty()) {
        bool connected = true;
        if (play(renderer, options.host, options.port, {options.character}, settings, options.debug_mode, connected)
            == Client::RunResult::Quit)
            return EXIT_SUCCESS;
        if (!connected)
            menu.setError("CAN'T REACH " + options.host + ":" + std::to_string(options.port));
    }

    menuLoop(renderer, menu, options.debug_mode);
    return EXIT_SUCCESS;
}
