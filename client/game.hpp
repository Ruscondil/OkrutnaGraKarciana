#pragma once
#include "connection.hpp"

class Game : public Handler, public ServerConnection
{
private:
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

public:
    virtual void handleEvent(uint32_t events) override;
    settings settings;
};
