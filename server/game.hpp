#pragma once
#include "client.hpp"
#include "handler.hpp"
#include "connection.hpp"

#include <map>
class Game : public Handler, public connectionManager
{
private:
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
    std::map<int, Client *> clients;

public:
    Game();
    virtual void handleEvent(uint32_t events, int _fd) override;
    void sendToAll(std::string eventName, std::string arguments);
    void newClient(int clientFd);
    void test(std ::string);
    void closeServer();
};
