#pragma once

#include "handler.hpp"
#include <sys/epoll.h>

#include <map>

class Client : public Handler
{

public:
    enum status
    {
        NOTAUTH = -2,
        NONICKNAME = -1,
        OK = 0,
        LOST = 1
    };
    explicit Client(int fd);
    int fd() const;
    void TriggerClientEvent(std::string eventName, std::string arguments);
    void TriggerClientEvent(std::string eventName);
    bool setNickname(std::string nickname);
    std::string getNickname() const;
    void setScore(int);
    void setScoreInc(int);
    int getScore() const;
    void setStatus(status);
    status getStatus() const;
    virtual void handleEvent(uint32_t events, int _fd) override;

private:
    int _fd;

    status _clientStatus;
    std::string _nickname;
    int _score;
};
