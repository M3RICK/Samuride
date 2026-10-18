/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#ifndef SOCKET_HPP
    #define SOCKET_HPP

#include <cstdint>
#include <vector>

// Non-blocking TCP helpers shared by the client and the server
namespace Socket {
    void setNonBlocking(int fd);

    // Appends whatever the socket has to `stream`. False once the peer is gone.
    bool receive(int fd, std::vector<uint8_t> &stream);

    bool send(int fd, const std::vector<uint8_t> &data);
}

#endif
