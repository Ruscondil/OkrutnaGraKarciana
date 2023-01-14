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
        int cardSets;
        // TODO wektor wektorów czarnych kart i białych
    };
    settings _settings;
    std::map<int, Client *> clients;
    int gameCzar;
    enum status
    {
        LOBBY = 0,
        ROUND = 1,
        ROUNDSUM = 2,
        END = 3
    };
    status gameStatus;
    void beginServerConnection(int, std::string arguments);
    void test(int, std ::string);
    void setPlayerNickname(int, std::string);
    void loadSettingsStartGame(int, std::string);

public:
    Game();
    virtual void handleEvent(uint32_t events, int _fd) override;
    void newClient(int clientFd);
    void removeClient(int clientFd);
    void closeClientFd(int clientFd);
    std::map<int, Client *>::iterator changeClientId(int clientFd, int newClientFD);
    std::map<int, Client *>::iterator changeClientId(int clientFd);
    void sendToAll(std::string eventName, std::string arguments);
    void sendToAllBut(int butID, std::string eventName, std::string arguments);
    void closeServer();
};
