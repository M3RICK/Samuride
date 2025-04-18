/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef DEBUG_HPP
    #define DEBUG_HPP

#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <mutex>

class Logger {
    private:
        bool debug_mode;
        std::mutex log_mutex;

    public:
        Logger(bool debug = false);

        void setDebugMode(bool mode);
        bool isDebugMode() const;
        void log(const std::string &msg);
        void logPacket(const std::string& direction, const char *data, size_t size);
};

extern Logger g_logger;

#define DEBUG_LOG(msg) g_logger.log(msg)
#define DEBUG_PACKET_SEND(data, size) g_logger.logPacket("SEND", data, size)
#define DEBUG_PACKET_RECV(data, size) g_logger.logPacket("RECV", data, size)

#endif
