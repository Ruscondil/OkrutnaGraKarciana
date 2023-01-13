#pragma once
#include "connection.hpp"
#include "handler.hpp"

class Game : public Handler, public ServerConnection
{
private:
    int clientServerFd;
    bool isReady;
    int round;
    struct settings
    {
        int roundTimeSeconds;
        int cardsOnHand;
        int pointsToWin;
        int blankCardCount;
        // TODO wektor wektorów czarnych kart i białych
    };
    void test(std::string);
    void beginClientConnection(std::string buffer);

public:
    Game();

    void TriggerServerEvent(std::string eventName, std::string arguments);
    virtual void handleEvent(uint32_t events) override;
    settings settings;
};
