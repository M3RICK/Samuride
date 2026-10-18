#ifndef SOCKET_HPP
    #define SOCKET_HPP

#include <cstdint>
#include <string>
#include <vector>

// TCP for the client and the server, on POSIX and Winsock alike.
// Platform types stay in socket.cpp: sockets are plain ints everywhere else.
namespace Socket {
    struct PollEntry {
        int fd;
        bool readable = false;
        bool closed = false;    // hung up or in error
    };

    // Winsock must be started once per process; nothing to do elsewhere
    bool startup();

    // Non-blocking listening socket, on loopback only or every interface. -1 on failure.
    int listen(int port, bool loopback_only);
    int accept(int listener);
    // Gives up after timeout_ms: nobody answering at an address would otherwise hang for minutes.
    // -1 on failure.
    int connect(const std::string &host, int port, int timeout_ms);
    void close(int fd);

    // Waits up to timeout_ms, then flags each entry. False on a real error.
    bool poll(std::vector<PollEntry> &entries, int timeout_ms);

    // Appends whatever the socket has to `stream`. False once the peer is gone.
    bool receive(int fd, std::vector<uint8_t> &stream);
    // Bytes the socket took, possibly fewer than all of them (0 when it's full), -1 on error
    int send(int fd, const std::vector<uint8_t> &data);

    std::string lastError();

    // This machine's address on the local network, what a friend should type to join
    std::string lanAddress();
}

#endif
