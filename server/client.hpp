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

public:
    Client(int fd);
    virtual ~Client(); // destuktor
    int fd() const;
    void TriggerClientEvent(std::string eventName);
    void changeNickname(std::string nickname);
    virtual void handleEvent(uint32_t events, int _fd) override;
};
