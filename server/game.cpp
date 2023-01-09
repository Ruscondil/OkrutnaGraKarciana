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
}

Game::settings::settings()
{
    roundTimeSeconds = 90;
    cardsOnHand = 6;
    pointsToWin = 3;
    blankCardCount = 5;
}

void Game::handleEvent(uint32_t events, int _fd)
{
    if (events & EPOLLIN)
    {
        char buffer[256] = "";

        ssize_t count = read(_fd, buffer, 256);
        if (count <= 0) // TODO do testów, zamienić na przenoszenie danego clienta w stan LOST
        {
            printf("Connection lost with %i\n", _fd);
            shutdown(getSocket(), SHUT_RDWR);
            // epoll_ctl(epollFd, EPOLL_CTL_DEL, STDIN_FILENO, &epollevent);
            close(getSocket());
            exit(0);
        }

        std::string s_buffer = std::string(buffer);
        if (count > 0)
        {
            std::string eventName = getEventName(s_buffer);
            std::string arguments = "TEST";

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
    std::string buffer = "";
    std::cout << "SIZE1 " << buffer.size() << std::endl;
    serializeInt(buffer, 2137);
    serializeString(buffer, "floppa friday i soggota");
    std::cout << "SIZE2 " << buffer.size() << std::endl;
    printText(buffer);
    // std::cout << "CZEMU ZAWSZE SA PROBLEMY" << std::endl;

    sendToAll(arg, buffer);
}

void Game::closeServer()
{
    for (auto const &client : clients)
        delete client.second;
    connectionManager::closeServer();
}