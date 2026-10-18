#ifndef PLATFORM_HPP
    #define PLATFORM_HPP

#include <filesystem>

// The few paths that differ between Linux, macOS and Windows
namespace Platform {
    // Folder of the running executable: the assets sit next to it
    std::filesystem::path executableDirectory();

    // Where settings and the best run are kept: ~/.local/share/samuride,
    // ~/Library/Application Support/Samuride on macOS, %APPDATA%\Samuride on Windows
    std::filesystem::path dataDirectory();
}

#endif
