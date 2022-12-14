#pragma once

#include <map>
#include <functional>

typedef std::function<void(std::string)> EventFunction;

class Handler
{
    std::map<std::string, EventFunction> eventInfo;

public:
    virtual ~Handler()
    {
    }
    virtual void handleEvent(uint32_t events) = 0;
    EventFunction getNetEventCallback(std::string);
    void registerNetEvent(std::string eventName, EventFunction);
    void eraseNetEvent(std::string eventName);
    void serializeEventName(std::string &buffer, std::string eventName);
    void serializeString(std::string &buffer, std::string value);
    void serializeInt(std::string &buffer, int value);
    int deserializeInt(std::string &buffer);
    std::string deserializeString(std::string &buffer);
    std::string getEventName(std::string &buffer);
};
