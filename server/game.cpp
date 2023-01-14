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
#include <sstream>
void printText(std::string text) // TODO usunać
{
    for (char c : text)
    {
        if (c == '\n')
        {
            std::cout << "\\n";
        }
        else if (c == '\r')
        {
            std::cout << "\\r";
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

void Game::handleEvent(uint32_t events, int source)
{
    if (events & EPOLLIN)
    {
        char buffer[256] = "";

        ssize_t count = read(source, buffer, 256);
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

            printf("Connection lost with %i\n", source);
            auto it = clients.find(source);
            if (it != clients.end())
            {
                if (gameStatus != LOBBY and it->second->getStatus() != Client::status::NOTAUTH and it->second->getStatus() != Client::status::NONICKNAME)
                {
                    it->second->setStatus(Client::status::LOST);
                    changeClientId(source);

                    closeClientFd(source);
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
    }
    if (events & ~EPOLLIN)
    {
        // remove();
    }
}

Game::Game() : gameStatus(LOBBY)
{
    registerNetEvent("testowanie", std::bind(&Game::test, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("beginServerConnection", std::bind(&Game::beginServerConnection, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("setPlayerNickname", std::bind(&Game::setPlayerNickname, this, std::placeholders::_1, std::placeholders::_2));
    registerNetEvent("loadSettingsStartGame", std::bind(&Game::loadSettingsStartGame, this, std::placeholders::_1, std::placeholders::_2));
}

void Game::newClient(int clientFd)
{
    clients[clientFd] = new Client(clientFd); // TODO dodać jakieś zabezpieczenie
    if (clients.size() == 1)
    {
        gameCzar = clientFd;
    }
    std::string message = "";
    serializeInt(message, clientFd);
    clients[clientFd]->TriggerClientEvent("beginClientConnection", message); // TODO giga zła głowa więc potem ogarnąć to find
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

void Game::closeClientFd(int clientFd)
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

void Game::test(int source, std::string arg)
{
    arg = "TEST";
    std::string buffer = "";
    serializeInt(buffer, 2137);
    serializeString(buffer, "floppa friday i soggota");
    // printText(buffer);
    //  std::cout << "CZEMU ZAWSZE SA PROBLEMY" << std::endl;

    sendToAll(arg, buffer);
}

void Game::beginServerConnection(int source, std::string arguments)
{ // TODO zmienainie statusu clienta
    std::cout << "Autoryzacja gracza ID " << source << std::endl;
    auto client = clients.find(source);
    client->second->setStatus(Client::status::NONICKNAME);
    std::string nicknames;
    for (auto const &x : clients)
    {
        if (x.second->getStatus() == Client::status::OK)
        {
            serializeString(nicknames, x.second->getNickname());
        }
    }
    client->second->TriggerClientEvent("showNicknameChoice", nicknames);
}

void Game::setPlayerNickname(int source, std::string arguments)
{
    auto client = clients.find(source);
    bool recoverClient = false;
    std::string message;
    std::string nickname = deserializeString(arguments);
    std::cout << "Gracz ID " << source << " ustawia nick na " << nickname << std::endl;

    for (auto const &x : clients)
    {
        if (x.second->getNickname() == nickname) // TODO może potem zmienić na to że musi być aktywny czy coś
        {
            if (x.second->getStatus() == Client::status::LOST)
            {
                std::cout << "Gracz ID " << source << " zostaje przywrócony do gry." << std::endl;
                delete client->second; // TODO zrobić na na funkcje czy ciś
                clients.erase(client);
                client = changeClientId(x.first, source);
                recoverClient = true;
            }
            else
            {
                error(0, 0, "Gracz ID %i probowal ustawic ustawic nickname \"%s\" gracza o ID %i", source, nickname.c_str(), x.first);
                std::string message;
                serializeString(message, "error");
                client->second->TriggerClientEvent("nicknameAcceptStatus", message);
                return;
            }
        }
    }
    message = "";
    serializeString(message, "ok");
    client->second->TriggerClientEvent("nicknameAcceptStatus", message);

    if (client->second->setNickname(nickname))
    {
        message = "";
        serializeString(message, nickname);
        sendToAllBut(source, "addPlayer", message);
        std::string nicknames;
        for (auto const &x : clients)
        {
            if (x.second->getNickname() != "") // TODO może potem zmienić na to że musi być aktywny czy coś
            {
                serializeString(nicknames, x.second->getNickname());
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

void Game::loadSettingsStartGame(int source, std::string arguments)
{

    if (source == gameCzar)
    {
        _settings.roundTimeSeconds = deserializeInt(arguments);
        _settings.cardsOnHand = deserializeInt(arguments);
        _settings.pointsToWin = deserializeInt(arguments);
        _settings.blankCardCount = deserializeInt(arguments);
        _settings.cardSets = deserializeInt(arguments);

        std::cout << "--------------START---------------" << std::endl;
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

        // TODO no wszystko co ma się dziać
        // Losowanie kart
        // Odpalenie timera
        // Wysłanie kto jest czarem
        //? Może to zrobić jako osobną funkcje bo przecież rudny też będą takie
        sendToAll("showGame", "");
    }
    else
    {
        error(0, 0, "Gracz ID %i próbował rozpocząc rozgrywkę nie będąc Hostem", source);
    }
}

void Game::newRound()
{
}

void Game::closeServer()
{
    for (auto const &client : clients)
        delete client.second;
    connectionManager::closeServer();
}