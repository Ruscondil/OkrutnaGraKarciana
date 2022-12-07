#pragma once
#include <sys/epoll.h>
#include <unordered_set>
#include <map>

extern int servFd;
extern int epollFd;

void ctrl_c(int); // TODO przenieść to gdzieś

void sendToAllBut(int fd, char *buffer, int count);

uint16_t readPort(char *txt);

void setReuseAddr(int sock);

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

class Client : public Handler
{
    int _fd;
    std::string _nickname;

public:
    Client(int fd);
    virtual ~Client(); // destuktor
    int fd() const;
    void TriggerClientEvent(std::string eventName);
    void changeNickname(std::string nickname);
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
