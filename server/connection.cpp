#include "connection.hpp"

#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

void connectionManager::reserveSocket()
{
    _servFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_servFd == -1)
        error(1, errno, "socket failed");
}

void connectionManager::setReuseAddr(int sock)
{
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (res)
        error(1, errno, "setsockopt failed");
}

void connectionManager::setReuseAddr()
{
    setReuseAddr(_servFd);
}

int connectionManager::getSocket() const
{
    return _servFd;
}

void connectionManager::setPort(char *txt)
{
    char *ptr;
    auto port = strtol(txt, &ptr, 10);
    if (*ptr != 0 || port < 1 || (port > ((1 << 16) - 1)))
        error(1, 0, "illegal argument %s", txt);
    _port = port;
}

void connectionManager::getReadyForConnection()
{
    sockaddr_in serverAddr{.sin_family = AF_INET, .sin_port = htons((short)_port), .sin_addr = {INADDR_ANY}};
    int res = bind(_servFd, (sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res)
        error(1, errno, "bind failed");

    res = listen(_servFd, 1);
    if (res)
        error(1, errno, "listen failed");
}

void connectionManager::prepareServer()
{
    reserveSocket();
    setReuseAddr();
    getReadyForConnection();
}

void connectionManager::setEpollFd(int epollFd)
{
    _epollFd = epollFd;
}

int connectionManager::getEpollFd() const
{
    return _epollFd;
}

void connectionManager::closeServer()
{
    shutdown(_servFd, SHUT_RDWR);
    close(_servFd);
    printf("Closing server\n");
    exit(0);
}