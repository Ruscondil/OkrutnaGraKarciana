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
