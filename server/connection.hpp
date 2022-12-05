#pragma once
#include <sys/epoll.h>
#include <unordered_set>

extern int servFd;
extern int epollFd;

void ctrl_c(int); // TODO przenieść to gdzieś

void sendToAllBut(int fd, char *buffer, int count);

uint16_t readPort(char *txt);

void setReuseAddr(int sock);

class Handler
{
public:
    virtual ~Handler() {}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client : public Handler
{
    int _fd;

public:
    Client(int fd);
    virtual ~Client(); // destuktor
    int fd() const;
    virtual void handleEvent(uint32_t events) override;
    void write(char *buffer, int count);
    void remove();
};

class ServHandler : Handler
{
public:
    virtual void handleEvent(uint32_t events) override;
};
extern ServHandler servHandler; // TODO poprawić tę abominację
extern std::unordered_set<Client *> clients;