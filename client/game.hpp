#pragma once
#include "connection.hpp"
#include "handler.hpp"

#include <map>

class Game : public Handler, public ServerConnection
{
private:
    int _clientServerFd;
    bool _isReady;
    int _round;
    struct settings
    {
        settings();
        int roundTimeSeconds;
        int cardsOnHand;
        int pointsToWin;
        int blankCardCount;
        int cardSets;
        // TODO wektor wektorów czarnych kart i białych
    };
    int _cardCzar;
    struct player
    {
        player();
        int score;
    };

    void test(std::string);
    void beginClientConnection(std::string buffer);
    void showNicknameChoice(std::string buffer);
    void setPlayers(std::string buffer);
    void addPlayer(std::string buffer);
    void sendSettingsStartGame();
    void setNickname();
    void nicknameAcceptStatus(std::string buffer);
    std::map<std::string, player *> players;
    settings _settings;

public:
    Game();

    void TriggerServerEvent(std::string eventName, std::string arguments);
    void TriggerServerEvent(std::string eventName);
    virtual void handleEvent(uint32_t events) override;
    void closeClient();
};
