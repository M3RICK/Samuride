/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "debug.hpp"

Logger::Logger(bool debug) : debug_mode(debug)
{}

void Logger::setDebugMode(bool mode)
{
    debug_mode = mode;
}

bool Logger::isDebugMode() const
{
    return debug_mode;
}

void Logger::log(const std::string &message)
{
    if (!debug_mode)
        return;

    std::lock_guard<std::mutex> lock(log_mutex);

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << message << std::endl;
}

void Logger::logPacket(const std::string &direction, const char *data, size_t size)
{
    if (!debug_mode)
        return;

    std::lock_guard<std::mutex> lock(log_mutex);

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << direction << " packet: ";

    for (size_t i = 0; i < std::min(size, static_cast<size_t>(16)); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (static_cast<int>(data[i]) & 0xFF) << " ";
    }

    if (size > 16) std::cout << "...";
    std::cout << std::dec << std::endl;
}

Logger g_logger;
