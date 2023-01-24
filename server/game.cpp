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

void printText(std::string text) // TODO usunać
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
        newfile << "roundTimeSeconds = 90" << std::endl;
        newfile << "cardsOnHand = 6" << std::endl;
        newfile << "pointsToWin = 3" << std::endl;
        newfile << "blankCardCount = 5" << std::endl;
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
            if (line.find("roundTimeSeconds") != std::string::npos)
            {
                _settings.roundTimeSeconds = std::stoi(line.substr(line.find("=") + 2));
            }
            else if (line.find("cardsOnHand") != std::string::npos)
            {
                _settings.cardsOnHand = std::stoi(line.substr(line.find("=") + 2));
            }
            else if (line.find("pointsToWin") != std::string::npos)
            {
                _settings.pointsToWin = std::stoi(line.substr(line.find("=") + 2));
            }
            else if (line.find("blankCardCount") != std::string::npos)
            {
                _settings.blankCardCount = std::stoi(line.substr(line.find("=") + 2));
            }
            else if (line.find("cardSets") != std::string::npos)
            {
                _settings.cardSets = std::stoi(line.substr(line.find("=") + 2));
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

        ssize_t count = read(source, buffer, 1024);
        std::string s_buffer;
        s_buffer.resize(count);
        memcpy(&s_buffer[0], buffer, count);

        if (count > 0)
        {
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
            events |= EPOLLERR;
            lostClient(source);
        }
    }
    if (events & ~EPOLLIN)
    {
        // remove();
    }
}

Game::Game() : gameStatus(LOBBY)
{
    registerNetEvent("beginServerConnection", std::bind(&Game::beginServerConnection, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("setPlayerNickname", std::bind(&Game::setPlayerNickname, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("loadSettingsStartGame", std::bind(&Game::loadSettingsStartGame, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("clientGetReady", std::bind(&Game::clientGetReady, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("pickAnswerSet", std::bind(&Game::pickAnswerSet, this, std::placeholders::_1, std::placeholders::_2));
    loadSettings();
}

void Game::lostClient(int source)
{
    auto client = clients.find(source);
    std::cout << "Stracono połączenie z graczem ID " << std::endl;
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

                    // TODO sprawdzić
                    newRound();
                }
            }
            else
            {
                if (gameStatus == ROUND)
                {
                    checkIfEveryoneReady();
                }
            }
        }
        else
        {
            removeClient(source);
        }
    }
    else
    {
        closeClientFd(source);
    }
}

void Game::newClient(int clientFd)
{
    clients[clientFd] = new Client(clientFd); // TODO dodać jakieś zabezpieczenie
    if (clients.size() == 1)
    {
        gameCzar = clientFd;
    }
    clients[clientFd]->TriggerClientEvent("beginClientConnection", serializeInt(clientFd)); // TODO giga zła głowa więc potem ogarnąć to find
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
        if (x.second->getNickname() == nickname) // TODO może potem zmienić na to że musi być aktywny czy coś
        {
            if (x.second->getStatus() == Client::status::LOST)
            {
                std::cout << "Gracz ID " << source << " zostaje przywrócony do gry." << std::endl;
                // TODO funkcja by przesłała graczowi najważniejsze info o grze
                returnedPlayer(source);
                delete client->second; // TODO zrobić na na funkcje czy ciś
                clients.erase(client);
                client = changeClientId(x.first, source);
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
        for (auto const &x : clients)
        {
            if (x.second->getNickname() != "") // TODO może potem zmienić na to że musi być aktywny czy coś
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
            return;
            // TODO wyrzucenie klienta
        }
    }

    client->second->setStatus(Client::status::OK);
}

void Game::returnedPlayer(int source)
{
    // TODO
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
            if (x.second->getNickname() != "") // TODO może potem zmienić na to że musi być aktywny czy coś
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

        // TODO no wszystko co ma się dziać
        // Odpalenie timera
        // Wysłanie kto jest czarem
        //? Może to zrobić jako osobną funkcje bo przecież rudny też będą takie
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
{ //! TODO Tutaj się robi deadlock nwm czemu jak nikogo nie wybrało w summary
    stopTimer();
    destroyTimer();
    gameStatus = ROUND;
    std::cout << "\n--------------NEW ROUND---------------" << std::endl;
    std::srand(unsigned(std::time(nullptr)));
    int blackCardIndex = std::rand() % calls.size(); // TODO dodać blank cardy
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
    // TODO Wybieranie card czara
    auto czar = clients.find(gameCzar); // TODO dać jakieś zabezpieczenie może
    // Uzupełnianie kart klientów do określoneej liczby

    for (auto const &x : clients)
    {
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
                // TODO zapamiętać jaką kartę dodaje i wysłać clientowi
                x.second->addCard(whiteCardIndex);
                addedCards.push_back(whiteCardIndex);
            }
            std::string arg = serializeInt(_settings.roundTimeSeconds) + serializeString(czar->second->getNickname());
            arg += serializeInt(getCallGaps(calls[blackCardIndex])) + serializeString(blackCard);
            for (int addedCard : addedCards)
            {
                arg += serializeInt(addedCard) + serializeString(responses[addedCard]);
            }
            // printText(arg);
            x.second->TriggerClientEvent("startRound", arg);
        }
    }

    startTimer(_settings.roundTimeSeconds);
    //! Jeżeli będzie za mało kart to się zapętli
}

void Game::clientGetReady(int source, std::string arguments)
{ // TODO dawać register i clear event
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
        if ((c.second->getStatus() == Client::status::OK and (!c.first) == gameCzar) and !c.second->getReady())
        {
            everyoneReady = false;
            break;
        }
    }
    if (everyoneReady)
    {
        std::cout << "Wszyscy gracze zgłosili gotowość, kończę rundę" << std::endl;
        stopTimer();
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
    std::string message; // TODO może zrobić zapamiętywanie wygenerowanej wiadomości?
    for (auto const &x : clients)
    {
        if ((x.second->getStatus() == Client::status::OK or x.second->getStatus() == Client::status::LOST) and x.second->pickedCardsCount() > 0)
        {
            message += serializeString(x.second->getNickname());
            int card = x.second->popPickedCard();
            while (card != -1)
            {
                message += serializeString(responses[card]);
                card = x.second->popPickedCard();
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
    std::string winnerNickname = deserializeString(arguments); //! Jak ktoś się połączony znowu/odłączy to zmieni mu się id TODO
    for (auto const &client : clients)
    {
        if ((client.second->getStatus() == Client::status::LOST or client.second->getStatus() == Client::status::OK) and client.second->getNickname() == winnerNickname)
        {
            client.second->setScoreInc(1);
            std::cout << "Gracz ID " << client.first << " " << winnerNickname << " zwyciężył rundę " << std::endl;
            if (client.second->getScore() >= _settings.pointsToWin)
            {
                std::cout << "Gracz ID " << client.first << " " << winnerNickname << " zwyciężył grę" << std::endl;
                sendToAll("info", "Grę zwyciężył" + winnerNickname);
                // todo Zrobić jakieś zakończenie czy coś
            }
            else
            {
                newRound();
            }
        }
    }
}

void Game::safeCloseServer()
{
    for (auto const &client : clients)
        delete client.second;
    stopTimer();
    destroyTimer();
    connectionManager::closeServer();
}