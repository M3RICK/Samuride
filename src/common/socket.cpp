/*
 ** EPITECH PROJECT, 2024
 ** B-NWP-jetpack
 ** File description:
 ** JETPACK
 */

#include "socket.hpp"
#include "debug.hpp"
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>

void Socket::setNonBlocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

bool Socket::receive(int fd, std::vector<uint8_t> &stream)
{
    uint8_t buffer[4096];
    ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return true;
    if (bytes_read <= 0)
        return false;

    DEBUG_PACKET_RECV(reinterpret_cast<const char *>(buffer), bytes_read);
    stream.insert(stream.end(), buffer, buffer + bytes_read);
    return true;
}

// MSG_NOSIGNAL: a peer that left must not kill us with SIGPIPE
// ponytail: a partial send on a full socket buffer is dropped, add a write queue if packets ever get big
bool Socket::send(int fd, const std::vector<uint8_t> &data)
{
    ssize_t bytes_sent = ::send(fd, data.data(), data.size(), MSG_NOSIGNAL);

    if (bytes_sent < 0) {
        DEBUG_LOG("Failed to send on fd " + std::to_string(fd) + ", errno=" + std::to_string(errno));
        return false;
    }

    DEBUG_PACKET_SEND(reinterpret_cast<const char *>(data.data()), bytes_sent);
    return true;
}
