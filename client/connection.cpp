#include "connection.hpp"
#include <errno.h>
#include <error.h>
#include <unistd.h>

ServerConnection::ServerConnection()
{
    _hints = {.ai_flags = 0, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    setServerAddress((char *)"127.0.0.1", (char *)"7777");
}

ServerConnection::ServerConnection(char *addr, char *port)
{
    _hints = {.ai_flags = 0, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    setServerAddress(addr, port);
}

ServerConnection::~ServerConnection() // destuktor
{   //TODO chyba useless
    // epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    // shutdown(_fd, SHUT_RDWR);
    close(_sock);
}

void ServerConnection::setServerAddress(char *addr, char *port)
{
    int res = getaddrinfo(addr, port, &_hints, &_resolved);
    if (res || !_resolved)
        error(1, 0, "getaddrinfo: %s", gai_strerror(res));
}

void ServerConnection::serverConnect()
{
    if (_resolved)
    {
        // create socket
        _sock = socket(_resolved->ai_family, _resolved->ai_socktype, 0);
        if (_sock == -1)
            error(1, errno, "socket failed");

        int res = connect(_sock, _resolved->ai_addr, _resolved->ai_addrlen);
        if (res)
            error(1, errno, "connect failed");

        // free memory
        freeaddrinfo(_resolved);
    }
}

void ServerConnection::serverConnect(char *addr, char *port)
{
    setServerAddress(addr, port);
    serverConnect();
}

int ServerConnection::getSocket()
{
    return _sock;
}

void ServerConnection::setEpollFd(int epollFd)
{
    _epollFd = epollFd;
}

int ServerConnection::getEpollFd() const
{
    return _epollFd;
}
