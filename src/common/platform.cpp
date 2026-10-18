/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "platform.hpp"
#include <cstdlib>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

std::filesystem::path Platform::executableDirectory()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(std::wstring(path, length)).parent_path();
#else
    std::error_code ec;
    return std::filesystem::canonical("/proc/self/exe", ec).parent_path();
#endif
}

std::filesystem::path Platform::dataDirectory()
{
#ifdef _WIN32
    const char *appdata = std::getenv("APPDATA");
    return std::filesystem::path(appdata ? appdata : ".") / "Samuride";
#else
    const char *home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / ".local/share/samuride";
#endif
}
