#pragma once

#include <netdb.h>
#include <map>

class ServerConnection
{
private:
    addrinfo *_resolved, _hints; // TCP
    int _sock;

public:
    ServerConnection();
    ServerConnection(char *addr, char *port);
    virtual ~ServerConnection();
    void setServerAddress(char *addr, char *port);
    void serverConnect();
    void serverConnect(char *addr, char *port);
    int getSocket();
};

class Handler
{
    std::map<std::string, bool> eventInfo;

public:
    virtual ~Handler()
    {
    }
    virtual void handleEvent(uint32_t events) = 0;
    bool doesEventExist(std::string eventName);
    void registerNetEvent(std::string eventName);
    void eraseNetEvent(std::string eventName);
};
