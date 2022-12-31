#pragma once
#include "client.hpp"
#include "handler.hpp"

#include <unordered_set>
class Game : public Handler
{
private:
    int _servFd;
    uint16_t _port;
    int _epollFd;
    struct settings
    {
        settings();
        int roundTimeSeconds;
        int cardsOnHand;
        int pointsToWin;
        int blankCardCount;
        // TODO wektor wektorów czarnych kart i białych
    };
    settings settings;
    std::unordered_set<Client *> clients;

public:
    virtual void handleEvent(uint32_t events, int _fd) override;
    void newClient(int clientFd);

    void reserveSocket();
    void setReuseAddr(int sock);
    void setReuseAddr();
    int getSocket();
    void sendToAll(int fd, char *buffer, int count);
    void setPort(char *txt);
    void getReadyForConnection();
    void prepareServer();
    void closeServer();
};
