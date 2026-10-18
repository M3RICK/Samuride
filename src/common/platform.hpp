/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef PLATFORM_HPP
    #define PLATFORM_HPP

#include <filesystem>

// The few paths that differ between Linux and Windows
namespace Platform {
    // Folder of the running executable: the assets sit next to it
    std::filesystem::path executableDirectory();

    // Where settings and the best run are kept: ~/.local/share/samuride, or %APPDATA%\Samuride
    std::filesystem::path dataDirectory();
}

#endif
