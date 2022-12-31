#include "handler.hpp"

void Handler::registerNetEvent(std::string eventName, EventFunction callback)
{
    if (!getNetEventCallback(eventName))
    {
        if (callback)
        {
            eventInfo[eventName] = callback;
        }
        else
        {
            // TODO dodać error
        }
    }
    else
    {
        // TODO dodać error
    }
}

void Handler::eraseNetEvent(std::string eventName)
{
    eventInfo.erase(eventName);
}

EventFunction Handler::getNetEventCallback(std::string eventName)
{
    if (eventInfo.find(eventName) != eventInfo.end())
        return eventInfo[eventName];
    return 0;
}