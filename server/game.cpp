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

        if (count <= 0) // TODO do testów, zamienić na przenoszenie danego clienta w stan LOST
        {
            printf("Connection lost with %i\n", source);
            shutdown(getSocket(), SHUT_RDWR);
            // epoll_ctl(epollFd, EPOLL_CTL_DEL, STDIN_FILENO, &epollevent);
            close(getSocket());
            exit(0);
        }

        if (count > 0)
        {
            printText(s_buffer);
            std::string eventName = getEventName(s_buffer);
            int clientAuth = deserializeInt(s_buffer);
            if (clientAuth != source)
            {
                error(0, 0, "Error: Wrong client code. Cliend ID: %i, Client auth code: %i", source, clientAuth);
                // TODO wywalenie użytkownika
            }
            std::cout << "count: " << count << " clientAuth: " << clientAuth << std::endl;
            std::string arguments = s_buffer;

            EventFunction callback = getNetEventCallback(eventName);
            if (callback)
            {
                callback(arguments);
            }
            else
            {
                error(0, 0, "Wrong event \"%s\", bit sus, clientID %i", eventName.c_str(), source);
            }
        }
        else
            events |= EPOLLERR;
    }
    if (events & ~EPOLLIN)
    {
        // remove();
    }
}

Game::Game()
{
    registerNetEvent("testowanie", std::bind(&Game::test, this, std::placeholders::_1));
    registerNetEvent("beginServerConnection", std::bind(&Game::beginServerConnection, this, std::placeholders::_1));
}

void Game::newClient(int clientFd)
{
    clients[clientFd] = new Client(clientFd); // TODO dodać jakieś zabezpieczenie
    std::string message = "";
    serializeInt(message, clientFd);
    clients[clientFd]->TriggerClientEvent("beginClientConnection", message); // TODO giga zła głowa więc potem ogarnąć to find
}

void Game::sendToAll(std::string eventName, std::string arguments)
{
    for (auto const &x : clients)
    {
        x.second->TriggerClientEvent(eventName, arguments);
    }
}

void Game::test(std::string arg)
{
    arg = "TEST";
    std::string buffer = "";
    serializeInt(buffer, 2137);
    serializeString(buffer, "floppa friday i soggota");
    printText(buffer);
    // std::cout << "CZEMU ZAWSZE SA PROBLEMY" << std::endl;

    sendToAll(arg, buffer);
}

void Game::beginServerConnection(std::string arguments)
{ // TODO zmienainie statusu clienta
    std::cout << "Autoryzacja gracza" << std::endl;
}

void Game::closeServer()
{
    for (auto const &client : clients)
        delete client.second;
    connectionManager::closeServer();
}