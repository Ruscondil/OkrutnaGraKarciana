#pragma once
#include "connection.hpp"
#include "handler.hpp"

#include <map>

class Game : public Handler, public ServerConnection
{
private:
    int _clientServerFd;
    bool _isReady;
    std::string _nickname;
    int _gameClock;
    struct settings
    {
        settings();
        int roundTimeSeconds;
        int cardsOnHand;
        int pointsToWin;
        int blankCardCount;
        int cardSets;
    };
    std::string _cardCzar;
    bool _isCardCzar;
    int _cardsCountToPick;

    struct player
    {
        player();
        int score;
    };
    std::map<int, std::string> cards;

    void beginClientConnection(std::string buffer);
    void showNicknameChoice(std::string buffer);
    void setPlayers(std::string buffer);
    void addPlayer(std::string buffer);
    void sendSettingsStartGame();
    void setNickname();
    void nicknameAcceptStatus(std::string buffer);
    void startRound(std::string buffer);
    void updateTimer(std::string buffer);
    void getReady();
    void receiveAnswers(std::string buffer);
    void pickAnswer();
    void showGame(std::string buffer);

    std::map<std::string, player *> players;
    settings _settings;

public:
    Game();

    void TriggerServerEvent(std::string eventName, std::string arguments);
    void TriggerServerEvent(std::string eventName);
    virtual void handleEvent(uint32_t events) override;
    void closeClient();
};
