#pragma once
#include "client.hpp"
#include "handler.hpp"
#include "connection.hpp"
#include "timer.hpp"

#include <map>
class Game : public Handler, public connectionManager, public Timer
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

    void beginServerConnection(std::string arguments);
    void test(std ::string);

public:
    Game();
    virtual void handleEvent(uint32_t events, int _fd) override;
    void newClient(int clientFd);
    void sendToAll(std::string eventName, std::string arguments);
    void closeServer();
};
