// TCP for the client and the server, on POSIX and Winsock alike

#include "socket.hpp"
#include "debug.hpp"
#include <cstring>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>

    // ponytail: SOCKET handles are stored as int; they fit in practice, switch to intptr_t if one ever doesn't
    using socklen_t = int;

    static int lastErrorCode() { return WSAGetLastError(); }
    static bool wouldBlock() { return WSAGetLastError() == WSAEWOULDBLOCK; }
    static bool connecting() { return WSAGetLastError() == WSAEWOULDBLOCK; }
    static int pollSockets(WSAPOLLFD *fds, size_t count, int timeout) { return WSAPoll(fds, count, timeout); }
    static constexpr int NO_SIGPIPE = 0;    // Winsock never raises SIGPIPE
    using PollFd = WSAPOLLFD;
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <sys/socket.h>
    #include <unistd.h>

    static int lastErrorCode() { return errno; }
    static bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK; }
    static bool connecting() { return errno == EINPROGRESS; }
    static int pollSockets(pollfd *fds, size_t count, int timeout) { return ::poll(fds, count, timeout); }
    // A peer that left must not kill us with SIGPIPE: per send on Linux,
    // per socket on macOS (SO_NOSIGPIPE, set in configure)
    #ifdef MSG_NOSIGNAL
    static constexpr int NO_SIGPIPE = MSG_NOSIGNAL;
    #else
    static constexpr int NO_SIGPIPE = 0;
    #endif
    using PollFd = pollfd;
#endif

bool Socket::startup()
{
#ifdef _WIN32
    static bool started = false;
    WSADATA data;

    if (!started)
        started = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    return started;
#else
    return true;
#endif
}

void Socket::close(int fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    ::close(fd);
#endif
}

// Every socket in use: non-blocking, and never raising SIGPIPE
static void configure(int fd)
{
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket(fd, FIONBIO, &on);
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    #ifdef SO_NOSIGPIPE
    int no_sigpipe = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
    #endif
#endif
}

// Writable means the connection settled: SO_ERROR tells whether it worked
static bool awaitConnection(int fd, int timeout_ms)
{
    PollFd entry = {static_cast<decltype(PollFd::fd)>(fd), POLLOUT, 0};
    int error = 0;
    socklen_t length = sizeof(error);

    if (pollSockets(&entry, 1, timeout_ms) <= 0)
        return false;
    return getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &length) == 0 && error == 0;
}

// The end of every socket setup: configured if it worked, closed if it didn't
static int finish(int fd, bool ok)
{
    if (!ok) {
        if (fd >= 0)
            Socket::close(fd);
        return -1;
    }
    configure(fd);
    return fd;
}

int Socket::listen(int port, bool loopback_only)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    sockaddr_in address = {};

    if (fd < 0)
        return -1;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(loopback_only ? INADDR_LOOPBACK : INADDR_ANY);
    address.sin_port = htons(port);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

    bool ok = bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0 && ::listen(fd, 5) == 0;
    return finish(fd, ok);
}

int Socket::accept(int listener)
{
    int fd = ::accept(listener, nullptr, nullptr);

    return finish(fd, fd >= 0);
}

// Non-blocking from the start: the connection completes in the background, polled until the timeout
int Socket::connect(const std::string &host, int port, int timeout_ms)
{
    addrinfo hints = {};
    addrinfo *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0)
        return -1;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    bool ok = false;

    if (fd >= 0) {
        configure(fd);
        ok = ::connect(fd, result->ai_addr, result->ai_addrlen) == 0 || (connecting() && awaitConnection(fd, timeout_ms));
    }
    freeaddrinfo(result);
    return finish(fd, ok);
}

bool Socket::poll(std::vector<PollEntry> &entries, int timeout_ms)
{
    std::vector<PollFd> fds;

    for (const PollEntry &entry : entries)
        fds.push_back({static_cast<decltype(PollFd::fd)>(entry.fd), POLLIN, 0});

    int ready = pollSockets(fds.data(), fds.size(), timeout_ms);
#ifndef _WIN32
    if (ready < 0 && errno == EINTR)
        ready = 0;
#endif
    if (ready < 0)
        return false;

    for (size_t i = 0; i < entries.size(); i++) {
        entries[i].readable = fds[i].revents & POLLIN;
        entries[i].closed = fds[i].revents & (POLLHUP | POLLERR);
    }
    return true;
}

bool Socket::receive(int fd, std::vector<uint8_t> &stream)
{
    char buffer[4096];
    int bytes_read = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_read < 0 && wouldBlock())
        return true;
    if (bytes_read <= 0)
        return false;

    DEBUG_PACKET_RECV(buffer, bytes_read);
    stream.insert(stream.end(), buffer, buffer + bytes_read);
    return true;
}

int Socket::send(int fd, const std::vector<uint8_t> &data)
{
    int bytes_sent = ::send(fd, reinterpret_cast<const char *>(data.data()), data.size(), NO_SIGPIPE);

    if (bytes_sent < 0 && wouldBlock())
        return 0;
    if (bytes_sent < 0) {
        DEBUG_LOG("Failed to send on fd " + std::to_string(fd) + ": " + lastError());
        return -1;
    }

    DEBUG_PACKET_SEND(reinterpret_cast<const char *>(data.data()), bytes_sent);
    return bytes_sent;
}

std::string Socket::lastError()
{
#ifdef _WIN32
    return "error " + std::to_string(lastErrorCode());
#else
    return std::strerror(lastErrorCode());
#endif
}

// "Connecting" a UDP socket sends nothing, it only picks the interface that would be used
std::string Socket::lanAddress()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in remote = {};
    sockaddr_in local = {};
    socklen_t length = sizeof(local);
    char text[INET_ADDRSTRLEN] = "127.0.0.1";

    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);
    inet_pton(AF_INET, "192.0.2.1", &remote.sin_addr);

    if (fd >= 0 && ::connect(fd, reinterpret_cast<sockaddr *>(&remote), sizeof(remote)) == 0 &&
        getsockname(fd, reinterpret_cast<sockaddr *>(&local), &length) == 0)
        inet_ntop(AF_INET, &local.sin_addr, text, sizeof(text));

    if (fd >= 0)
        close(fd);
    return text;
}
