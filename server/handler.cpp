#include "handler.hpp"
#include <cstring>
#include <arpa/inet.h>
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

void Handler::serializeInt(std::string &buffer, int value)
{
    char data[4];
    value = htonl(value);
    memcpy(data, &value, 4);
    buffer.append(" " + std::string(data, 4));
}

int Handler::deserializeInt(std::string &buffer)
{
    // Get the data from the buffer (skip the space preceding it)
    std::string data = buffer.substr(1, 4);

    // Convert the data back to an int
    int value;
    memcpy(&value, data.c_str(), 4);

    // Convert back to host byte order
    value = ntohl(value);

    // Delete the space and data from the buffer
    buffer.erase(0, 5);

    return value;
}

std::string Handler::getEventName(std::string &buffer)
{
    std::string eventName;
    for (int i = 0; i < (int)buffer.size(); i++)
    {
        if (buffer[i] == ' ')
        {
            buffer.erase(0, i + 1); // delete first word and space from buffer
            return eventName;
        }
        else
        {
            eventName += buffer[i];
        }
    }
    buffer.clear(); // if there is no space in buffer, delete everything
    return eventName;
}