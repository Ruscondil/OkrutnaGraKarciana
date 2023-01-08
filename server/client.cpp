#include "client.hpp"
#include "handler.hpp"
// #include <cstdlib>
// #include <cstdio>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>
#include <iostream>
#include <cstring>

Client::Client(int fd) : _fd(fd)
{
    // epoll_event ee{EPOLLIN | EPOLLRDHUP, {.ptr = this}};
    // epoll_ctl(_epollFd, EPOLL_CTL_ADD, _fd, &ee); // TODO naprawić
    // registerNetEvent("a", &Client::Test);
}

Client::~Client() // destuktor
{
    // epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
}

int Client::fd() const { return _fd; }

void Client::changeNickname(std::string nickname)
{
    _nickname = nickname;
}

void Client::handleEvent(uint32_t events, int _fd)
{
}

/* void Client::remove()
{
    printf("removing %d\n", _fd);
    clients.erase(this);
    delete this;
} */

void Client::TriggerClientEvent(std::string eventName, std::string arguments)
{
    std::string message;
    std::cout << "TRIGGER " << _fd << std::endl;

    message = eventName + arguments;

    int count = message.length();

    char test[256]; // TODO no zmienić by nie był test i dać size taki jaki powinien być
    memcpy(test, message.data(), message.size());
    if (count != ::write(_fd, test, count))
    {
        _clientStatus = LOST;
    }
    // remove();
    // TODO zmienić status na niektywny
}
