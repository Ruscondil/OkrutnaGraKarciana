#pragma once

#include <map>
#include <functional>

typedef std::function<void(std::string)> EventFunction;

class Handler
{
    std::map<std::string, EventFunction> eventInfo; // TODO będą sobie rozpakowywać funkcje już we wnętrzu

public:
    virtual ~Handler()
    {
    }
    virtual void handleEvent(uint32_t events, int _fd) = 0;
    EventFunction getNetEventCallback(std::string);
    void registerNetEvent(std::string eventName, EventFunction);
    void eraseNetEvent(std::string eventName);
};
