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
#include <signal.h>
#include <iostream>
#include <cstring>

Client::Client(int fd) : _fd(fd)
{
    _nickname = "";
    _score = 0;
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

bool Client::setNickname(std::string nickname)
{
    if (_nickname == "") // TODO zmienić by było zależne od statusu
    {
        _nickname = nickname;
        return true;
    }
    return false;
}

std::string Client::getNickname()
{
    return _nickname;
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
    if (!TriggerEvent(_fd, eventName, arguments))
    {
    }
    // TODO zmienić status na niektywny
}

void Client::TriggerClientEvent(std::string eventName)
{
    TriggerClientEvent(eventName, "");
}

int Client::getScore()
{
    return _score;
}

void Client::setScore(int score)
{
    _score = score;
}

void Client::setScoreInc(int inc)
{
    _score = _score + inc;
}