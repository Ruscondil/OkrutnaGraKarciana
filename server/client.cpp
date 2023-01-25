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

Client::Client(int fd) : _fd(fd), _clientStatus(NOTAUTH), _nickname(""), _isReady(false), _score(0) {}

int Client::fd() const { return _fd; }

bool Client::setNickname(std::string const nickname)
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

void Client::TriggerClientEvent(std::string const eventName, std::string const arguments)
{
    TriggerEvent(_fd, eventName, arguments);
}

void Client::TriggerClientEvent(std::string const eventName)
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

void Client::addCard(int id)
{
    cardsID.insert(id);
}
void Client::deleteCard(int id)
{
    cardsID.erase(id);
}
int Client::getCardsCount()
{
    return cardsID.size() - 1;
}
bool Client::haveCard(int id)
{
    return cardsID.find(id) != cardsID.end();
}

void Client::setReady(bool value)
{
    _isReady = value;
}
bool Client::getReady()
{
    return _isReady;
}

bool Client::pickCard(int id)
{
    if (!haveCard(id))
    {
        return false;
    }
    else
    {
        cardsIDPicked.push_back(id);
        return true;
    }
}
void Client::clearPickedCards()
{
    cardsIDPicked.clear();
}

std::vector<int> Client::getPickedCards()
{
    return cardsIDPicked;
}

int Client::pickedCardsCount()
{
    return cardsIDPicked.size();
}
