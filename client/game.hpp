#pragma once
#include "connection.hpp"
#include "handler.hpp"

#include <map>
#include <vector>
#include <utility>
#include <queue>

typedef std::function<void(std::string)> InputFunction;

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

    std::string blackCard;
    int _cardsCountToPick;

    struct player
    {
        player();
        int score;
        bool isOnline;
        std::vector<std::string> cardsPicked;

        std::vector<std::string> getPickedCards();
        void addPickedCard(std::string card);
        void deletePickedCards();
        int getPickedCardsCount();
    };
    std::map<std::string, player *> players;

    void beginClientConnection(std::string buffer);
    void showNicknameChoice(std::string buffer);
    void setPlayers(std::string buffer);
    void addPlayer(std::string buffer);
    void sendSettingsStartGame(std::string buffer);
    void setNickname(std::string);
    void nicknameAcceptStatus(std::string buffer);
    void startRound(std::string buffer);
    void updateTimer(std::string buffer);
    void getReady(std::string buffer);
    void receiveAnswers(std::string buffer);
    void pickAnswer(std::string buffer);
    void showGame();

    InputFunction inputCallback;
    void setInputCallack(InputFunction, std::string);
    void setInputCallack(InputFunction);
    void setInputCallack();

    std::vector<std::pair<int, std::string>> cards;
    void addCard(std::pair<int, std::string>);
    int getCardsCount();
    void deleteCard(int index);

    std::vector<std::string> playersInSummary;
    void addSummaryPlayer(std::string nickname);
    std::vector<std::string> getSummaryPlayers();
    bool isInSummaryPlayers(int id);
    std::string getSummaryPlayer(int id);
    void deleteSummaryPlayers();

    void showAnswers();

    void info(std::string);

    settings _settings;

public:
    Game();

    void TriggerServerEvent(std::string eventName, std::string arguments);
    void TriggerServerEvent(std::string eventName);
    virtual void handleEvent(uint32_t events) override;
    void handleInput(std::string);
    void closeClient();
};
