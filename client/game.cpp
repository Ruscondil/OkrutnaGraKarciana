#include "game.hpp"
#include "connection.hpp"
#include "handler.hpp"

#include <iostream>
#include <sys/epoll.h> //epoll
#include <unistd.h>    //read
#include <functional>  //std::bind
#include <cstring>     //memcpy
#include <error.h>
#include <vector>
#include <sstream>

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

void Game::handleInput(std::string buffer)
{
    if (inputCallback)
    {
        inputCallback(buffer);
    }
    // std::cout << "TEST: " << buffer << std::endl;
}

void Game::setInputCallack(InputFunction callback, std::string message)
{
    if (message.size() > 0)
    {
        std::cout << message << std::endl;
    }
    inputCallback = callback;
}

void Game::setInputCallack(InputFunction callback)
{
    setInputCallack(callback, "");
}
void Game::setInputCallack()
{
    setInputCallack(NULL, "");
}

Game::Game()
{
    registerNetEvent("beginClientConnection", std::bind(&Game::beginClientConnection, this, std::placeholders::_1));
    registerNetEvent("showNicknameChoice", std::bind(&Game::showNicknameChoice, this, std::placeholders::_1));
    registerNetEvent("addPlayer", std::bind(&Game::addPlayer, this, std::placeholders::_1));
    registerNetEvent("setPlayers", std::bind(&Game::setPlayers, this, std::placeholders::_1));
    registerNetEvent("nicknameAcceptStatus", std::bind(&Game::nicknameAcceptStatus, this, std::placeholders::_1));
    // registerNetEvent("showGame", std::bind(&Game::showGame, this, std::placeholders::_1));
    registerNetEvent("startRound", std::bind(&Game::startRound, this, std::placeholders::_1));
    registerNetEvent("updateTimer", std::bind(&Game::updateTimer, this, std::placeholders::_1));
    registerNetEvent("receiveAnswers", std::bind(&Game::receiveAnswers, this, std::placeholders::_1));
    registerNetEvent("info", std::bind(&Game::info, this, std::placeholders::_1));
}

Game::player::player() : score(0) {}

Game::settings::settings() : roundTimeSeconds(90), cardsOnHand(6), pointsToWin(3), blankCardCount(5), cardSets(6) {}

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
    setInputCallack(std::bind(&Game::setNickname, this, std::placeholders::_1), "Wpisz swój nickname");
}

void Game::setNickname(std::string nickname)
{
    for (int i = 0; i < static_cast<int>(nickname.length()); i++)
    {
        if (nickname[i] == ' ' || nickname[i] == '\t' || nickname[i] == '\n' || nickname[i] == '\r')
        {
            error(0, 0, "W nicku nie mogą być białe znaki");
            return;
        }
    }
    // Spradzanie czy nick nie jest zajęty
    if (players.find(nickname) != players.end())
    {
        error(0, 0, "Taki nick juz zostal zajety");
        return;
    }

    _nickname = nickname;
    TriggerServerEvent("setPlayerNickname", serializeString(nickname));
}

void Game::nicknameAcceptStatus(std::string buffer)
{
    std::string message = deserializeString(buffer);

    if (message == "ok")
    {
        bool isHost = deserializeInt(buffer);
        if (isHost)
        {
            setInputCallack(std::bind(&Game::sendSettingsStartGame, this, std::placeholders::_1), "Napisz \"ready\" jak wszyscy będą gotowi");
        }
        else
        {
            setInputCallack();
        }

        // TODO wyświetlenie lobby
    }
    else if (message == "error")
    {
        // TODO wyświetlenie błędu
        error(0, 0, "Nick juz jest zajety");
        setInputCallack(std::bind(&Game::setNickname, this, std::placeholders::_1), "Wpisz nickname ponownie");
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

void Game::sendSettingsStartGame(std::string buffer)
{
    if (buffer == "ready")
    {
        setInputCallack();
        std::string message;
        message += serializeInt(_settings.roundTimeSeconds) + serializeInt(_settings.cardsOnHand) + serializeInt(_settings.pointsToWin);
        message += serializeInt(_settings.blankCardCount) + serializeInt(_settings.cardSets);
        TriggerServerEvent("loadSettingsStartGame", message);
    }
    else
    {
        std::cout << "Napisz \"ready\" jak wszyscy będą gotowi" << std::endl;
    }
}

void Game::addCard(std::pair<int, std::string> card)
{
    cards.push_back(card);
}
void Game::deleteCard(int index)
{
    cards.erase(cards.begin() + index);
}

void Game::showGame()
{
    std::cout << "Card Czar: " << _cardCzar << std::endl;
    std::cout << "Hasło: " << blackCard << std::endl;
    if (!_isCardCzar)
    {
        std::cout << "Posiadane karty" << std::endl;
        for (int i = 0; i < static_cast<int>(cards.size()); i++)
        {
            std::cout << i + 1 << ". " << cards[i].second << std::endl;
        }
    }
    else
    {
        std::cout << "Jesteś card czarem, poczekaj aż inni wybiorą karty" << std::endl;
    }
}

void Game::startRound(std::string buffer)
{
    for (auto const &x : players)
    {
        // TODO czyczenie queue każdemy graczowi
    }

    _gameClock = deserializeInt(buffer);
    _cardCzar = deserializeString(buffer);
    _isCardCzar = (_cardCzar == _nickname);

    _cardsCountToPick = deserializeInt(buffer);
    blackCard = deserializeString(buffer);

    while (buffer.size() > 0)
    {
        int cardID = deserializeInt(buffer);
        std::string card = deserializeString(buffer);
        addCard(make_pair(cardID, card));
    }
    showGame();
    if (!_isCardCzar)
    {
        setInputCallack(std::bind(&Game::getReady, this, std::placeholders::_1), "Wybierz " + std::to_string(_cardsCountToPick) + " kart po indexie");
    }
}

void Game::updateTimer(std::string buffer)
{
    int newTime = deserializeInt(buffer);
    if (newTime < _gameClock)
    {
        _gameClock = newTime;
    }
    if (_gameClock == 60)
    {
        std::cout << "Została 1 min" << std::endl;
    }
    else if (_gameClock == 30)
    {
        std::cout << "Została 30 sekund" << std::endl;
    }
    else if (_gameClock == 15)
    {
        std::cout << "Zostało 15 sekund" << std::endl;
    }
    else if (_gameClock == 5)
    {
        std::cout << "Zostało 5 sekund" << std::endl;
    }
}

void Game::getReady(std::string buffer)
{

    std::vector<int> result;
    std::stringstream ss(buffer);
    int num;
    while (ss >> num)
    {
        result.push_back(num + 1);
    }
    if (static_cast<int>(result.size()) != _cardsCountToPick)
    {
        error(0, 0, "Zła liczba odpowiedzi");
        return;
    }
    std::cout << "test" << std::endl;
    std::string message;
    for (int i = 0; i < static_cast<int>(result.size()); i++)
    {
        message += serializeInt(cards[result[i]].first); // TODO usuwanie kart z zasobnika
    }
    for (int i = 0; i < static_cast<int>(result.size()); i++)
    {
        deleteCard(result[i]); //! TODO co jak się przestawi index?
    }

    TriggerServerEvent("clientGetReady", message);
}

void Game::showAnswers()
{
    for (auto const &x : players)
    {
        if (x.second->cardsPicked.size() > 0) // TODO zmienić na funkcje
        {
            std::cout << "Gracz " << x.first << std::endl;
            for (int i = 0; i < _cardsCountToPick; i++)
            {
                std::cout << i + 1 << ". " << x.second->cardsPicked.front() << std::endl;
                x.second->cardsPicked.pop();
            }
        }
    }
}

void Game::receiveAnswers(std::string buffer)
{
    while (buffer.size() > 0)
    {
        std::string nickname = deserializeString(buffer);
        auto cardOwner = players.find(nickname);
        if (cardOwner != players.end())
        {
            for (int i = 0; i < _cardsCountToPick; i++)
            {
                std::string answer = deserializeString(buffer);
                std::cout << "TEST " << answer << std::endl;
                cardOwner->second->cardsPicked.push(answer); // TODO zmienić na funkcję
            }
        }
        else
        {
            error(0, 0, "Błąd przy wczytywaniu odpowiedzi. Brak gracza %s", nickname.c_str());
            for (int i = 0; i < _cardsCountToPick; i++)
            {
                deserializeString(buffer);
            }
            return;
        }
    }
    // TODO wymyślić jak dzielić odpowiedzi na od danego gracza
    // Może na początku wysłać ile jest black kart a potem ściągać odp
    showAnswers();
    if (_isCardCzar)
    {
        setInputCallack(std::bind(&Game::pickAnswer, this, std::placeholders::_1), "Wybierz gracza, którego odpowiedzi ci się najbardziej podobają");
    }
    else
    {
        setInputCallack();
    }
}

void Game::pickAnswer(std::string buffer)
{
    if (_isCardCzar)
    {
        auto winner = players.find(buffer);
        if (winner != players.end())
        {
            //! TODO zrobić by nie usuwać rzeczy z queue
            // if (winner->second->cardsPicked.size() > 0) // TODO zmienić na funkcje
            //{
            TriggerServerEvent("pickAnswerSet", serializeString(buffer));
            setInputCallack();
            return;
            //}
            // else
            //{
            //    error(0, 0, "Został wybrany gracz bez odpowiedzi");
            //    setInputCallack(std::bind(&Game::pickAnswer, this, std::placeholders::_1), "Ponownie wybierz gracza, którego odpowiedzi ci się najbardziej podobają");
            //    return;
            // }
        }
        else
        {
            error(0, 0, "Taki gracz nie istnieje");
            setInputCallack(std::bind(&Game::pickAnswer, this, std::placeholders::_1), "Ponownie wybierz gracza, którego odpowiedzi ci się najbardziej podobają");
            return;
        }
    }
    else
    {
        error(0, 0, "Próbowano wybrać odpowiedź, nie będąc Card Czarem");
        return;
    }
}

void Game::info(std::string buffer)
{
    std::string message = deserializeString(buffer);
    if (message.size() > 0)
    {
        std::cout << message << std::endl;
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