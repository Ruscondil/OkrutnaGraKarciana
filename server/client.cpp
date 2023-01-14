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

Client::Client(int fd) : _fd(fd), _clientStatus(NOTAUTH), _nickname(""), _score(0) {}

int Client::fd() const { return _fd; }

bool Client::setNickname(std::string nickname)
{
    if (getStatus() == NONICKNAME)
    {
        _nickname = nickname;
        return true;
    }
    return false;
}

std::string Client::getNickname() const
{
    return _nickname;
}

void Client::handleEvent(uint32_t events, int _fd)
{
}

void Client::TriggerClientEvent(std::string eventName, std::string arguments)
{
    if (!TriggerEvent(_fd, eventName, arguments))
    {
        // TODO dać że user lost
    }
}

void Client::TriggerClientEvent(std::string eventName)
{
    TriggerClientEvent(eventName, "");
}

int Client::getScore() const
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

void Client::setStatus(status newStatus)
{
    _clientStatus = newStatus;
}

Client::status Client::getStatus() const
{
    return _clientStatus;
}