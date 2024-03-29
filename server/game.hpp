#pragma once
#include "client.hpp"
#include "handler.hpp"
#include "connection.hpp"
#include "timer.hpp"
#include "cardImport.hpp"

#include <map>
#include <vector>

class Game : public Handler, public connectionManager, public Timer, public CardImporter
{
private:
    struct settings
    {
        settings();
        int roundTimeSeconds;
        int cardsOnHand;
        int pointsToWin;
        int blankCardCount;
        int cardSets;
    };

    std::vector<std::vector<std::string>> calls;
    std::vector<std::string> responses;

    settings _settings;
    std::map<int, Client *> clients;
    int gameCzar;
    int blackCardIndex;
    enum status
    {
        LOBBY = 0,
        ROUND = 1,
        ROUNDSUM = 2,
        END = 3
    };
    status gameStatus;
    void beginServerConnection(int source, std::string arguments);
    void setPlayerNickname(int source, std::string arguments);
    void startGame(int source, std::string arguments);
    void clientGetReady(int source, std::string arguments);
    void pickAnswerSet(int source, std::string arguments);

    void lostClient(int id);
    void returnedPlayer(int id);

    void loadSettings();

    int newCardCzar(int oldCzar);
    void newRound();
    void checkIfEveryoneReady();
    void startSummary();

    virtual void secondPassed(int) override;
    virtual void timerDone() override;

public:
    Game();
    virtual void handleEvent(uint32_t events, int _fd) override;
    void newClient(int clientFd);
    void removeClient(int clientFd);
    void closeClientFd(int clientFd) const;
    std::map<int, Client *>::iterator changeClientId(int clientFd, int newClientFD);
    std::map<int, Client *>::iterator changeClientId(int clientFd);
    void sendToAll(std::string eventName, std::string arguments);
    void sendToAllBut(int butID, std::string eventName, std::string arguments);
    void safeCloseServer();
};
