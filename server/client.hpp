#pragma once

#include "handler.hpp"
#include <sys/epoll.h>

#include <map>

class Client : public Handler
{
    int _fd;
    enum status
    {
        OK = 0,
        LOST = 1

    };
    status _clientStatus;
    std::string _nickname;
    int _score;

public:
    Client(int fd);
    virtual ~Client(); // destuktor
    int fd() const;
    void TriggerClientEvent(std::string eventName, std::string arguments);
    void TriggerClientEvent(std::string eventName);
    bool setNickname(std::string nickname);
    std::string getNickname();
    void setScore(int);
    void setScoreInc(int);
    int getScore();
    virtual void handleEvent(uint32_t events, int _fd) override;
};
