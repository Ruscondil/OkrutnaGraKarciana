#include "handler.hpp"
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <error.h>

bool Handler::TriggerEvent(int reciverFd, std::string eventName, std::string arguments)
{
    std::string message;
    std::cout << "TRIGGER " << reciverFd << " " << eventName << std::endl;
    message = eventName + arguments;
    int count = message.length();
    char test[256]; // TODO no zmienić by nie był test i dać size taki jaki powinien być
    memcpy(test, message.data(), message.size());
    return count != ::write(reciverFd, test, count);

    // TODO zmienić status na niektywny
}

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
            error(0, 0, "Null callback function given");
        }
    }
    else
    {
        error(0, 0, "Event already exists");
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

void Handler::serializeEventName(std::string &buffer, std::string eventName)
{
    buffer.append(eventName + "\n");
}

void Handler::serializeInt(std::string &buffer, int value)
{
    char data[4];
    value = htonl(value);
    memcpy(data, &value, 4);
    if (buffer.size() > 0)
        buffer.pop_back(); // usuwanie \n
    buffer.append("\r" + std::string(data, 4) + "\n");
}

void Handler::serializeString(std::string &buffer, std::string value)
{
    if (buffer.size() > 0)
        buffer.pop_back(); // usuwanie \n
    buffer.append("\r" + value + "\n");
}

int Handler::deserializeInt(std::string &buffer)
{
    // Get the data from the buffer (skip the space preceding it)
    std::string data = buffer.substr(0, 4);

    // Convert the data back to an int
    int value;
    memcpy(&value, data.c_str(), 4);

    // Convert back to host byte order
    value = ntohl(value);

    // Delete the /r and data from the buffer
    buffer.erase(0, 5);

    return value;
}

std::string Handler::deserializeString(std::string &buffer)
{
    std::string text;
    for (int i = 0; i < (int)buffer.size(); i++)
    {
        if (buffer[i] == '\r' or buffer[i] == '\n')
        {
            buffer.erase(0, i + 1); // delete first word and space from buffer
            return text;
        }
        else
        {
            text += buffer[i];
        }
    }
    buffer.clear(); // if there is no space in buffer, delete everything
    return text;
}

std::string Handler::getEventName(std::string &buffer)
{
    return deserializeString(buffer);
}