#include "connection.hpp"

//#include <cstdlib>
//#include <cstdio>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>
#include <iostream>

int servFd;
int epollFd;
ServHandler servHandler;
std::unordered_set<Client *> clients;

uint16_t readPort(char *txt)
{
    char *ptr;
    auto port = strtol(txt, &ptr, 10);
    if (*ptr != 0 || port < 1 || (port > ((1 << 16) - 1)))
        error(1, 0, "illegal argument %s", txt);
    return port;
}

void setReuseAddr(int sock)
{
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (res)
        error(1, errno, "setsockopt failed");
}

void sendToAllBut(int fd, char *buffer, int count)
{
    auto it = clients.begin();
    while (it != clients.end())
    {
        Client *client = *it;
        it++;
        if (client->fd() != fd)
            client->write(buffer, count);
    }
}

Client::Client(int fd) : _fd(fd)
{
    epoll_event ee{EPOLLIN | EPOLLRDHUP, {.ptr = this}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
}

Client::~Client() // destuktor
{
    epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
}

int Client::fd() const { return _fd; }

void Client::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        char buffer[256];
        ssize_t count = read(_fd, buffer, 256);
        if (count > 0)
            sendToAllBut(_fd, buffer, count);
        else
            events |= EPOLLERR;
    }
    if (events & ~EPOLLIN)
    {
        remove();
    }
}
void Client::write(char *buffer, int count)
{
    if (count != ::write(_fd, buffer, count))
        remove();
}
void Client::remove()
{
    printf("removing %d\n", _fd);
    clients.erase(this);
    delete this;
}

void ServHandler::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        sockaddr_in clientAddr{};
        socklen_t clientAddrSize = sizeof(clientAddr);

        auto clientFd = accept(servFd, (sockaddr *)&clientAddr, &clientAddrSize);
        if (clientFd == -1)
            error(1, errno, "accept failed");

        printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);

        clients.insert(new Client(clientFd));
    }
    if (events & ~EPOLLIN)
    {
        error(0, errno, "Event %x on server socket", events);
        ctrl_c(SIGINT);
    }
}

void ctrl_c(int)
{
    for (Client *client : clients)
        delete client;
    close(servFd);
    printf("Closing server\n");
    exit(0);
}