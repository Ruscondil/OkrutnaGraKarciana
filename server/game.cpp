#include "game.hpp"
#include "connection.hpp"

#include <errno.h>
#include <error.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <functional>
#include <cstring>
#include <vector>
#include <fstream>

void printText(std::string text) // For testing
{
    for (char c : text)
    {
        if (c == '\a')
        {
            std::cout << "\\a";
        }
        else if (c == '\t')
        {
            std::cout << "\\t";
        }
        else if (c == '\n')
        {
            std::cout << "\\n";
        }
        else if (c == '\r')
        {
            std::cout << "\\r";
        }
        else if (c == '\\')
        {
            std::cout << "\\";
        }
        else if (c == '\?')
        {
            std::cout << "\?";
        }
        else
        {
            std::cout << c;
        }
    }
    std::cout << std::endl;
}

Game::settings::settings()
{
    roundTimeSeconds = 90;
    cardsOnHand = 6;
    pointsToWin = 3;
    blankCardCount = 5;
    cardSets = 1;
}

void Game::loadSettings()
{
    std::ifstream file("settings.conf");
    if (!file)
    {
        std::ofstream newfile("settings.conf");
        // newfile << "roundTimeSeconds = 90" << std::endl;
        newfile << "cardsOnHand = 6" << std::endl;
        newfile << "pointsToWin = 3" << std::endl;
        // newfile << "blankCardCount = 5" << std::endl;
        newfile << "cardSets = 6" << std::endl;
        newfile.close();
        _settings.roundTimeSeconds = 90;
        _settings.cardsOnHand = 6;
        _settings.pointsToWin = 3;
        _settings.blankCardCount = 5;
        _settings.cardSets = 6;
    }
    else
    {
        std::string line;
        while (getline(file, line))
        {
            /*  if (line.find("roundTimeSeconds") != std::string::npos)
             {
                 _settings.roundTimeSeconds = std::stoi(line.substr(line.find("=") + 2));
             }
             else */
            if (line.find("cardsOnHand") != std::string::npos)
            {
                _settings.cardsOnHand = std::stoi(line.substr(line.find("=") + 2));
                if (_settings.cardsOnHand < 3 or _settings.cardsOnHand > 10)
                {
                    std::cout << "Niepoprawna wartość cardsOnHand, ustawiam domyślną" << std::endl;
                    _settings.cardsOnHand = 7;
                }
            }
            else if (line.find("pointsToWin") != std::string::npos)
            {

                _settings.pointsToWin = std::stoi(line.substr(line.find("=") + 2));
                if (_settings.pointsToWin <= 0 or _settings.pointsToWin > 10)
                {
                    std::cout << "Niepoprawna wartość pointsToWin, ustawiam domyślną" << std::endl;
                    _settings.pointsToWin = 3;
                }
            }
            /*
            else if (line.find("blankCardCount") != std::string::npos)
            {
                _settings.blankCardCount = std::stoi(line.substr(line.find("=") + 2));
            }
            */
            else if (line.find("cardSets") != std::string::npos)
            {
                _settings.cardSets = std::stoi(line.substr(line.find("=") + 2));
                if (_settings.cardSets <= 0 or _settings.cardSets > 7)
                {
                    std::cout << "Niepoprawna wartość cardSets, ustawiam domyślną" << std::endl;
                    _settings.cardSets = 6;
                }
            }
        }
    }
    file.close();
}

void Game::handleEvent(uint32_t events, int source)
{
    if (events & EPOLLIN)
    {
        char buffer[1024] = "";

        ssize_t count = recv(source, buffer, 1024, 0);
        if (count > 0)
        {
            std::string s_buffer;
            s_buffer.resize(count);
            memcpy(&s_buffer[0], buffer, count);

            while (s_buffer.size() > 0)
            {
                // printText(s_buffer);
                std::string eventName = getEventName(s_buffer);
                int clientAuth = deserializeInt(s_buffer);
                std::string arguments = getArguments(s_buffer);

                if (clientAuth != source)
                {
                    error(0, 0, "Error: Wrong client auth code. Cliend ID: %i, Client auth code: %i", source, clientAuth);
                    removeClient(source);
                    return;
                }

                EventFunction callback = getNetEventCallback(eventName);
                if (callback)
                {
                    callback(source, arguments);
                }
                else
                {
                    error(0, 0, "Wrong event \"%s\", bit sus, clientID %i", eventName.c_str(), source);
                }
            }
        }
        else
        {
            lostClient(source);
        }
    }
}

Game::Game() : gameStatus(LOBBY)
{
    registerNetEvent("beginServerConnection", std::bind(&Game::beginServerConnection, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("setPlayerNickname", std::bind(&Game::setPlayerNickname, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("startGame", std::bind(&Game::startGame, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("clientGetReady", std::bind(&Game::clientGetReady, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("pickAnswerSet", std::bind(&Game::pickAnswerSet, this, std::placeholders::_1, std::placeholders::_2));
    loadSettings();
}

void Game::lostClient(int source)
{
    auto client = clients.find(source);
    std::cout << "Stracono połączenie z graczem ID " << source << std::endl;
    if (client != clients.end())
    {
        if (gameStatus != LOBBY and client->second->getStatus() != Client::status::NOTAUTH and client->second->getStatus() != Client::status::NONICKNAME)
        {
            client->second->setStatus(Client::status::LOST);
            changeClientId(source);
            closeClientFd(source);
            if (gameCzar == source)
            {
                if (gameStatus == ROUND or gameStatus == ROUNDSUM)
                {
                    newRound();
                    return;
                }
            }
            else
            {
                if (gameStatus == ROUND)
                {
                    checkIfEveryoneReady();
                    return;
                }
            }
        }
        else
        {
            removeClient(source);
            return;
        }
    }
    else
    {
        closeClientFd(source);
        return;
    }
}

void Game::newClient(int clientFd)
{
    auto client = clients.find(clientFd);
    if (client == clients.end())
    {
        clients[clientFd] = new Client(clientFd);
        if (clients.size() == 1)
        {
            gameCzar = clientFd;
        }
        clients[clientFd]->TriggerClientEvent("beginClientConnection", serializeInt(clientFd));
    }
    else
    {
        error(1, 0, "Gracz o takim deskryptorze %i już istnieje w bazie", clientFd);
    }
}

void Game::removeClient(int clientFd)
{
    std::cout << "Usuwanie Gracza ID " << clientFd << std::endl;
    auto it = clients.find(clientFd);
    if (it != clients.end())
    {
        delete it->second;
        clients.erase(it);
        closeClientFd(clientFd);
    }
    else
    {
        error(0, 0, "Usuwanie gracza ID %i nie udane. Gracz nie istnieje", clientFd);
    }
}

void Game::closeClientFd(int clientFd) const
{
    epoll_ctl(getEpollFd(), EPOLL_CTL_DEL, clientFd, nullptr);
    shutdown(clientFd, SHUT_RDWR);
    close(clientFd);
}

std::map<int, Client *>::iterator Game::changeClientId(int clientFd, int newClientFd)
{
    if (clients.find(newClientFd) == clients.end() and clients.find(clientFd) != clients.end())
    {
        auto nodeHandler = clients.extract(clientFd);
        nodeHandler.key() = newClientFd;
        clients.insert(std::move(nodeHandler));
        return clients.find(newClientFd);
    }
    else
    {
        error(1, 0, "Zamiana ID niemożliwa. Stare ID %i, Nowe ID %i", clientFd, newClientFd);
        return clients.end();
    }
}

std::map<int, Client *>::iterator Game::changeClientId(int clientFd)
{
    int key = -1;
    while (clients.count(key) != 0)
    {
        key--;
    }
    return changeClientId(clientFd, key);
}

void Game::sendToAll(std::string eventName, std::string arguments)
{
    for (auto const &x : clients)
    {
        x.second->TriggerClientEvent(eventName, arguments);
    }
}

void Game::sendToAllBut(int butID, std::string eventName, std::string arguments)
{
    for (auto const &x : clients)
    {
        if (x.first != butID)
            x.second->TriggerClientEvent(eventName, arguments);
    }
}

void Game::beginServerConnection(int source, std::string const arguments)
{
    std::cout << "Autoryzacja gracza ID " << source << std::endl;
    auto client = clients.find(source);
    client->second->setStatus(Client::status::NONICKNAME);
    std::string nicknames;
    for (auto const &x : clients)
    {
        if (x.second->getStatus() == Client::status::OK)
        {
            nicknames += serializeString(x.second->getNickname());
        }
    }
    client->second->TriggerClientEvent("showNicknameChoice", nicknames);
}

void Game::setPlayerNickname(int source, std::string arguments)
{
    auto client = clients.find(source);
    bool recoverClient = false;
    std::string nickname = deserializeString(arguments);
    std::cout << "Gracz ID " << source << " ustawia nick na " << nickname << std::endl;

    for (auto const &x : clients)
    {
        if (x.second->getNickname() == nickname)
        {
            if (x.second->getStatus() == Client::status::LOST)
            {
                std::cout << "Gracz ID " << source << " zostaje przywrócony do gry." << std::endl;
                delete client->second;
                clients.erase(client);
                client = changeClientId(x.first, source);
                returnedPlayer(source);
                recoverClient = true;
            }
            else
            {
                error(0, 0, "Gracz ID %i probowal ustawic ustawic nickname \"%s\" gracza o ID %i", source, nickname.c_str(), x.first);

                client->second->TriggerClientEvent("nicknameAcceptStatus", serializeString("error"));
                return;
            }
        }
    }

    client->second->TriggerClientEvent("nicknameAcceptStatus", serializeString("ok") + serializeInt(source == gameCzar));

    if (client->second->setNickname(nickname))
    {
        sendToAllBut(source, "addPlayer", serializeString(nickname));
        std::string nicknames;
        client->second->setStatus(Client::status::OK);
        for (auto const &x : clients)
        {
            if (x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST)
            {
                nicknames += serializeString(x.second->getNickname());
            }
        }
        client->second->TriggerClientEvent("setPlayers", nicknames);
    }
    else
    {
        if (!recoverClient)
        {
            error(0, 0, "Gracz ID %i probowal ponownie ustawic nickname", source);
            lostClient(source);
            return;
        }
    }
}

void Game::returnedPlayer(int source)
{
    auto client = clients.find(source);
    std::string nicknames;
    client->second->setStatus(Client::status::OK);
    for (auto const &x : clients)
    {
        if (x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST)
        {
            nicknames += serializeString(x.second->getNickname());
        }
    }
    client->second->TriggerClientEvent("setPlayers", nicknames);
    if (gameStatus == ROUND)
    {
        std::string blackCard = cardIntoString(calls[blackCardIndex]);

        auto czar = clients.find(gameCzar);

        std::string arg = serializeInt(false) + serializeInt(_settings.roundTimeSeconds) + serializeString(czar->second->getNickname());
        arg += serializeInt(getCallGaps(calls[blackCardIndex])) + serializeString(blackCard);
        auto clientCards = client->second->getCards();
        for (int clientCard : clientCards)
        {
            arg += serializeInt(clientCard) + serializeString(responses[clientCard]);
        }
        client->second->TriggerClientEvent("startRound", arg);
    }
    else if (gameStatus == ROUNDSUM)
    {
        std::string blackCard = cardIntoString(calls[blackCardIndex]);

        auto czar = clients.find(gameCzar);

        std::string arg = serializeInt(true) + serializeInt(_settings.roundTimeSeconds) + serializeString(czar->second->getNickname());
        arg += serializeInt(getCallGaps(calls[blackCardIndex])) + serializeString(blackCard);
        auto clientCards = client->second->getCards();
        for (int clientCard : clientCards)
        {
            arg += serializeInt(clientCard) + serializeString(responses[clientCard]);
        }
        client->second->TriggerClientEvent("startRound", arg);

        std::string message;
        message += serializeInt(getCallGaps(calls[blackCardIndex]));
        for (auto const &x : clients)
        {
            if ((x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST) and x.second->pickedCardsCount() > 0)
            {
                auto karty = x.second->getPickedCards();
                message += serializeString(x.second->getNickname());

                for (int i = static_cast<int>(karty.size()) - 1; i >= 0; i--)
                {
                    message += serializeString(responses[karty[i]]);
                }
            }
        }
        if (message.size() > 0)
        {
            client->second->TriggerClientEvent("receiveAnswers", message);
        }
    }
}

void Game::startGame(int source, std::string arguments)
{
    if (gameStatus != LOBBY)
    {
        error(0, 0, "Gracz ID %i próbował ponownie wystartować grę", source);
        return;
    }
    if (source == gameCzar)
    {
        std::cout << "\n--------------START---------------" << std::endl;
        std::cout << "----Gracze----" << std::endl;
        for (auto const &x : clients)
        {
            if (x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST)
            {
                std::cout << x.second->getNickname() << std::endl;
            }
        }
        std::cout << "----Ustawienia----" << std::endl;
        std::cout << "roundTimeSeconds: " << _settings.roundTimeSeconds << std::endl;
        std::cout << "cardsOnHand: " << _settings.cardsOnHand << std::endl;
        std::cout << "pointsToWin: " << _settings.pointsToWin << std::endl;
        std::cout << "blankCardCount: " << _settings.blankCardCount << std::endl;
        std::cout << "cardSets: " << _settings.cardSets << std::endl;

        if (_settings.cardSets & 1)
        {
            loadFromFile("decks/set_example1.json");
            std::vector<std::vector<std::string>> deckCalls = getCalls();
            calls.insert(calls.end(), deckCalls.begin(), deckCalls.end());
            std::vector<std::string> deckResponses = getResponses();
            responses.insert(responses.end(), deckResponses.begin(), deckResponses.end());
        }
        if (_settings.cardSets & 2)
        {
            loadFromFile("decks/set_example2.json");
            std::vector<std::vector<std::string>> deckCalls = getCalls();
            calls.insert(calls.end(), deckCalls.begin(), deckCalls.end());
            std::vector<std::string> deckResponses = getResponses();
            responses.insert(responses.end(), deckResponses.begin(), deckResponses.end());
        }
        if (_settings.cardSets & 4)
        {
            loadFromFile("decks/set_example3.json");
            std::vector<std::vector<std::string>> deckCalls = getCalls();
            calls.insert(calls.end(), deckCalls.begin(), deckCalls.end());
            std::vector<std::string> deckResponses = getResponses();
            responses.insert(responses.end(), deckResponses.begin(), deckResponses.end());
        }
        newRound();
    }
    else
    {
        error(0, 0, "Gracz ID %i próbował rozpocząc rozgrywkę nie będąc Hostem", source);
    }
}

int Game::newCardCzar(int oldCzar)
{
    auto it = clients.find(oldCzar);
    if (it == clients.end())
    {
        for (auto first = clients.begin(); first != clients.end(); ++first)
        {
            if (first->second->getStatus() == Client::status::OK)
                return first->first;
        }
    }
    else
    {
        for (auto next = ++it; next != clients.end(); ++next)
        {
            if (next->second->getStatus() == Client::status::OK)
                return next->first;
        }
        for (auto first = clients.begin(); first != it; ++first)
        {
            if (first->second->getStatus() == Client::status::OK)
                return first->first;
        }
    }
    return -1; // może być -1 bo <0 w mapie przyjmują tylko LOST
}

void Game::newRound()
{ //!  Tutaj się robi deadlock, ponieważ przy timerDone, wszystko jest dalej odpalane w timerze przez co przy destroyTimer nie można czekać na koniec timer bo funkcja jest w timerze.
    //! Do rozwiązania brak timera
    // stopTimer();
    // destroyTimer();
    gameStatus = ROUND;
    std::cout << "\n--------------NEW ROUND---------------" << std::endl;
    std::srand(unsigned(std::time(nullptr)));
    blackCardIndex = std::rand() % calls.size();
    std::string blackCard = cardIntoString(calls[blackCardIndex]);
    std::cout << "Blanks: " << getCallGaps(calls[blackCardIndex]) << " Call: " << blackCard << std::endl;
    gameCzar = newCardCzar(gameCzar);

    if (gameCzar < 0)
    {
        std::cout << "Nie można było znaleźć kolejnego Card Czara, zamykam grę" << std::endl;
        safeCloseServer();
    }
    else
    {
        std::cout << "Card Czarem zostaje GRACZ ID " << gameCzar << std::endl;
    }
    auto czar = clients.find(gameCzar);

    for (auto const &x : clients)
    {
        x.second->setReady(false);
        x.second->clearPickedCards(); // czyszczenie zdobytych kart

        if (x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST)
        {
            std::vector<int> addedCards;
            while (x.second->getCardsCount() < _settings.cardsOnHand)
            {
                bool noOneHas = false;
                int whiteCardIndex;
                while (!noOneHas)
                {
                    noOneHas = true;
                    whiteCardIndex = std::rand() % responses.size();
                    for (auto const &y : clients)
                    {
                        if ((y.second->getStatus() == Client::status::OK or y.second->getStatus() == Client::status::LOST) and y.second->haveCard(whiteCardIndex))
                        {
                            noOneHas = false;
                            break;
                        }
                    }
                }
                x.second->addCard(whiteCardIndex);
                addedCards.push_back(whiteCardIndex);
            }
            std::string arg = serializeInt(false) + serializeInt(_settings.roundTimeSeconds) + serializeString(czar->second->getNickname());
            arg += serializeInt(getCallGaps(calls[blackCardIndex])) + serializeString(blackCard);
            for (int addedCard : addedCards)
            {
                arg += serializeInt(addedCard) + serializeString(responses[addedCard]);
            }
            x.second->TriggerClientEvent("startRound", arg);
        }
    }

    // startTimer(_settings.roundTimeSeconds);
    //! Jeżeli będzie za mało kart w puli to się zapętli
}

void Game::clientGetReady(int source, std::string arguments)
{
    auto client = clients.find(source);
    std::cout << "Gracz ID " << source << " " << client->second->getNickname() << " jest gotowy z odpowiedzami" << std::endl;
    while (arguments.size() > 0)
    {
        int cardID = deserializeInt(arguments);

        if (!client->second->pickCard(cardID))
        {
            error(1, 0, "Gracz ID %i próbuję wykorzystać kartę, której nie posiada", source);
            return;
        }
        client->second->deleteCard(cardID);
        std::cout << responses[cardID] << std::endl;
    }
    client->second->setReady(true);
    checkIfEveryoneReady();
}

void Game::checkIfEveryoneReady()
{
    bool everyoneReady = true;
    for (auto const &c : clients)
    {
        if (c.second->getStatus() == Client::status::OK and !(c.first == gameCzar) and !c.second->getReady())
        {
            everyoneReady = false;
            break;
        }
    }
    if (everyoneReady)
    {
        std::cout << "Wszyscy gracze zgłosili gotowość, kończę rundę" << std::endl;
        // stopTimer();
        startSummary();
    }
}

void Game::secondPassed(int timeLeft)
{
    for (auto const &client : clients)
    {
        if (client.second->getStatus() == Client::status::OK)
        {
            client.second->TriggerClientEvent("updateTimer", serializeInt(timeLeft));
        }
    }
}

void Game::timerDone()
{
    startSummary();
}

void Game::startSummary()
{
    gameStatus = ROUNDSUM;
    std::string message;
    message += serializeInt(getCallGaps(calls[blackCardIndex]));
    for (auto const &x : clients)
    {
        if ((x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST) and x.second->pickedCardsCount() > 0)
        {
            auto karty = x.second->getPickedCards();
            message += serializeString(x.second->getNickname());

            for (int i = static_cast<int>(karty.size()) - 1; i >= 0; i--)
            {
                message += serializeString(responses[karty[i]]);
            }
        }
    }
    if (message.size() > 0)
    {
        for (auto const &x : clients)
        {
            if (x.second->getStatus() == Client::status::OK)
            {
                x.second->TriggerClientEvent("receiveAnswers", message);
            }
        }
    }
    else
    {
        std::cout << "Nikt nie dostarczył odpowowiedzi na czas" << std::endl;
        sendToAll("info", serializeString("Nikt nie dostarczył odpowowiedzi na czas"));
        newRound();
        // TODO co jeśli nikt nie dał kart
    }
}

void Game::pickAnswerSet(int source, std::string arguments)
{
    std::string winnerNickname = deserializeString(arguments);
    for (auto const &client : clients)
    {
        if ((client.second->getStatus() == Client::status::LOST or client.second->getStatus() == Client::status::OK) and client.second->getNickname() == winnerNickname)
        {
            client.second->setScoreInc(1);
            std::cout << "Gracz ID " << client.first << " " << winnerNickname << " zwyciężył rundę " << std::endl;
            sendToAll("info", serializeString("Rudnę zwyciężył gracz o nicku " + winnerNickname));
            if (client.second->getScore() >= _settings.pointsToWin)
            {
                std::cout << "Gracz ID " << client.first << " " << winnerNickname << " zwyciężył grę" << std::endl;
                sendToAll("info", serializeString("Grę zwyciężył gracz o nicku " + winnerNickname));
            }
            else
            {
                newRound();
            }
            return;
        }
    }
    error(1, 0, "Gracz wybrany jako zwycięzca nie istnieje %s", winnerNickname.c_str());
}

void Game::safeCloseServer()
{
    for (auto const &client : clients)
        delete client.second;
    // stopTimer();
    // destroyTimer();
    connectionManager::closeServer();
}