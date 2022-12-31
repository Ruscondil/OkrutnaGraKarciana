#pragma once

#include <map>

typedef void (*EventFunction)(std::string);

class Handler
{
    std::map<std::string, EventFunction> eventInfo; // TODO będą sobie rozpakowywać funkcje już we wnętrzu

public:
    virtual ~Handler()
    {
    }
    virtual void handleEvent(uint32_t events) = 0;
    EventFunction getNetEventCallback(std::string);
    void registerNetEvent(std::string eventName, EventFunction);
    void eraseNetEvent(std::string eventName);
};
