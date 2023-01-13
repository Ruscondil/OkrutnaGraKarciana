#pragma once

#include <map>
#include <functional>

typedef std::function<void(int, std::string)> EventFunction;

class Handler
{
    std::map<std::string, EventFunction> eventInfo;

public:
    virtual ~Handler()
    {
    }
    virtual void handleEvent(uint32_t events, int source) = 0;
    bool TriggerEvent(int reciverFd, std::string eventName, std::string arguments);
    EventFunction getNetEventCallback(std::string);
    void registerNetEvent(std::string eventName, EventFunction);
    void eraseNetEvent(std::string eventName);
    void serializeString(std::string &buffer, std::string value);
    void serializeInt(std::string &buffer, int value);
    int deserializeInt(std::string &buffer);
    std::string deserializeString(std::string &buffer);
    std::string getEventName(std::string &buffer);
    std::string getArguments(std::string &buffer);
};
