#pragma once
#include <string>
class connectionManager
{
private:
    int _servFd;
    uint16_t _port;
    int _epollFd;

public:
    void reserveSocket();
    void setReuseAddr(int sock);
    void setReuseAddr();
    int getSocket();
    void setPort(char *txt);
    void setEpollFd(int epollFd);
    int getEpollFd() const;
    void getReadyForConnection();
    void prepareServer();
    void closeServer();
};
