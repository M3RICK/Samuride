// The few paths that differ between Linux, macOS and Windows

#include "platform.hpp"
#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

std::filesystem::path Platform::executableDirectory()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(std::wstring(path, length)).parent_path();
#elif defined(__APPLE__)
    char path[4096];
    uint32_t size = sizeof(path);
    std::error_code ec;
    if (_NSGetExecutablePath(path, &size) != 0)
        return ".";
    return std::filesystem::canonical(path, ec).parent_path();
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
#elif defined(__APPLE__)
    const char *home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "Library/Application Support/Samuride";
#else
    const char *home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / ".local/share/samuride";
#endif
}
