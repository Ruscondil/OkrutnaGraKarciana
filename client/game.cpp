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
            std::cout << "\\?";
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
        char buffer[1024] = "";
        ssize_t count = read(getSocket(), buffer, 1024);
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
            closeClient();
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
    registerNetEvent("startRound", std::bind(&Game::startRound, this, std::placeholders::_1));
    registerNetEvent("updateTimer", std::bind(&Game::updateTimer, this, std::placeholders::_1));
    registerNetEvent("receiveAnswers", std::bind(&Game::receiveAnswers, this, std::placeholders::_1));
}

Game::player::player() : score(0) {}

Game::settings::settings() : roundTimeSeconds(90), cardsOnHand(6), pointsToWin(3), blankCardCount(5), cardSets(6) {}

void Game::test(std::string a)
{
    int test;
    std::string test2;
    // printText(a);
    test = deserializeInt(a);
    test2 = deserializeString(a);
    std::cout << test << " | " << test2 << std::endl;
}

void Game::TriggerServerEvent(std::string const eventName, std::string arguments)
{
    // printText(message + arguments);
    TriggerEvent(getSocket(), eventName, serializeInt(_clientServerFd) + arguments);
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
    std::string nickname;
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
    _nickname = nickname;
    TriggerServerEvent("setPlayerNickname", serializeString(nickname));
    if (nickname == "start")
    {
        sendSettingsStartGame();
    }
}

void Game::nicknameAcceptStatus(std::string buffer)
{
    printText(buffer);
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
    message += serializeInt(_settings.roundTimeSeconds) + serializeInt(_settings.cardsOnHand) + serializeInt(_settings.pointsToWin);
    message += serializeInt(_settings.blankCardCount) + serializeInt(_settings.cardSets);
    TriggerServerEvent("loadSettingsStartGame", message);
}

void Game::startRound(std::string buffer)
{
    _gameClock = deserializeInt(buffer);
    _cardCzar = deserializeString(buffer);
    _isCardCzar = (_cardCzar == _nickname);
    std::cout << "Nick: " << _cardCzar << std::endl;
    int ileBlack = deserializeInt(buffer);
    std::string blackcard = deserializeString(buffer);
    std::cout << ileBlack << " " << blackcard << std::endl;
    while (buffer.size() > 0)
    {
        int cardID = deserializeInt(buffer);
        std::string card = deserializeString(buffer);
        cards[cardID] = card; // TODO to chyba i tak będzie wysyłanie i przechowowywane po stronie frontu

        std::cout << card << std::endl;
    }
    getReady();
}

void Game::updateTimer(std::string buffer)
{
    int newTime = deserializeInt(buffer);
    if (newTime < _gameClock)
    {
        std::cout << newTime << std::endl;
        _gameClock = newTime;
    }
}

void Game::getReady()
{
    std::string message;
    auto it = cards.begin();
    for (int i = 0; i < 1; i++)
    {
        message += serializeInt(it->first);
        it++;
    }

    TriggerServerEvent("clientGetReady", message);
}

void Game::receiveAnswers(std::string buffer)
{ // TODO wymyślić jak dzielić odpowiedzi na od danego gracza
  // Może na początku wysłać ile jest black kart a potem ściągać odp
}

void Game::pickAnswer()
{
    if (_isCardCzar)
    {
    }
    else
    {
        error(0, 0, "Próbowano wybrać odpowiedź, nie będąc Card Czarem");
    }
}

void Game::closeClient()
{
    shutdown(getSocket(), SHUT_RDWR);
    epoll_ctl(getEpollFd(), EPOLL_CTL_DEL, getSocket(), nullptr);
    close(getSocket());
    printf("Closing client\n");
    exit(0);
}