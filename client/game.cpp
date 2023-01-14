#include "game.hpp"
#include "connection.hpp"
#include "handler.hpp"

#include <iostream>
#include <sys/epoll.h> //epoll
#include <unistd.h>    //read
#include <functional>  //std::bind
#include <cstring>     //memcpy
#include <error.h>

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

void Game::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        char buffer[256] = "";
        ssize_t count = read(getSocket(), buffer, 256);
        // std::cout << "Count: " << count << std::endl;
        std::string s_buffer; // copying every bit of char to string
        s_buffer.resize(count);
        memcpy(&s_buffer[0], buffer, count);

        if (count > 0)
        {
            while (s_buffer.size() > 0)
            {
                // printText(s_buffer);
                std::string eventName = getEventName(s_buffer);
                std::string arguments = getArguments(s_buffer);
                // printText(s_buffer);
                EventFunction callback = getNetEventCallback(eventName);
                if (callback)
                {
                    callback(arguments);
                }
                else
                {
                    error(0, 0, "Wrong event \"%s\", bit sus", eventName.c_str());
                }
            }
        }
        else if (count <= 0)
        {
            events |= EPOLLERR;
            printf("Connection lost\n");
            shutdown(getSocket(), SHUT_RDWR);
            // epoll_ctl(epollFd, EPOLL_CTL_DEL, STDIN_FILENO, &epollevent);
            close(getSocket());
            exit(0);
        }
    }
    if (events & ~EPOLLIN)
    {
        // TODO
    }
}

Game::Game()
{
    registerNetEvent("TEST", std::bind(&Game::test, this, std::placeholders::_1));
    registerNetEvent("beginClientConnection", std::bind(&Game::beginClientConnection, this, std::placeholders::_1));
    registerNetEvent("showNicknameChoice", std::bind(&Game::showNicknameChoice, this, std::placeholders::_1));
    registerNetEvent("addPlayer", std::bind(&Game::addPlayer, this, std::placeholders::_1));
    registerNetEvent("setPlayers", std::bind(&Game::setPlayers, this, std::placeholders::_1));
    registerNetEvent("nicknameAcceptStatus", std::bind(&Game::nicknameAcceptStatus, this, std::placeholders::_1));
}

Game::player::player() : score(0) {}

Game::settings::settings() : roundTimeSeconds(90), cardsOnHand(6), pointsToWin(3), blankCardCount(5), cardSets(1) {}

void Game::test(std::string a)
{
    int test;
    std::string test2;
    // printText(a);
    test = deserializeInt(a);
    test2 = deserializeString(a);
    std::cout << test << " | " << test2 << std::endl;
}

void Game::TriggerServerEvent(std::string eventName, std::string arguments)
{
    std::string message;
    serializeInt(message, _clientServerFd);
    // printText(message + arguments);
    TriggerEvent(getSocket(), eventName, message + arguments);
}

void Game::TriggerServerEvent(std::string eventName)
{
    TriggerServerEvent(eventName, "");
}

void Game::beginClientConnection(std::string buffer)
{
    _clientServerFd = deserializeInt(buffer);
    TriggerServerEvent("beginServerConnection");
}

void Game::showNicknameChoice(std::string buffer)
{
    players.clear();
    while (buffer.size() > 0)
    {
        std::string nickname = deserializeString(buffer);
        players[nickname] = new player();
    }
    setNickname();
}

void Game::setNickname()
{
    std::string message, nickname;
    bool zle = true;
    // Spradzanie czy nick nie jest zajęty
    while (zle)
    {
        std::cin >> nickname;
        if (players.find(nickname) != players.end())
            error(0, 0, "Taki nick juz zostal zajety");
        else
            zle = false;
    }

    serializeString(message, nickname);
    TriggerServerEvent("setPlayerNickname", message);
    if (nickname == "start")
    {
        sendSettingsStartGame();
    }
}

void Game::nicknameAcceptStatus(std::string buffer)
{
    std::string message = deserializeString(buffer);

    if (message == "ok")
    {
        // TODO wyświetlenie lobby
    }
    else if (message == "error")
    {
        // TODO wyświetlenie błędu
        error(0, 0, "Nick juz jest zajety");
        setNickname();
    }
    else
    {
        error(1, 0, "nicknameAcceptStatus: zły argument");
    }
}

void Game::addPlayer(std::string buffer)
{
    std::string nickname = deserializeString(buffer);
    players[nickname] = new player();
}

void Game::setPlayers(std::string buffer)
{

    players.clear();
    while (buffer.size() > 0)
    {
        std::string nickname = deserializeString(buffer);
        players[nickname] = new player();
    }
}

void Game::sendSettingsStartGame()
{
    std::string message;
    serializeInt(message, _settings.roundTimeSeconds);
    serializeInt(message, _settings.cardsOnHand);
    serializeInt(message, _settings.pointsToWin);
    serializeInt(message, _settings.blankCardCount);
    serializeInt(message, _settings.cardSets);
    TriggerServerEvent("loadSettingsStartGame", message);
}
