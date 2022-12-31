#include "game.hpp"
#include <errno.h>
#include <error.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
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
        if (count > 0)
        { // TODO deserializacja
            std::string eventName = buffer;
            // std::cout << "Wrong event, bit sus: " << count << " " << eventName << std::endl;
            if (true)
            {
                if (eventName == "player_registerNickname")
                {
                    std::cout << "Wykonuję" << std::endl; // TODO usunąć
                }
                else
                {
                    std::cout << "NOT FOUND: Execute of event: " << eventName << std::endl;
                }
            }
            else
            { // TODO sprawdzanie globalnej mapy
                if (true)
                {
                }
                else
                {
                    std::cout << "Wrong event, bit sus: " << count << " " << eventName << std::endl;
                    // remove();
                }
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

void Game::newClient(int clientFd)
{
    clients.insert(new Client(clientFd));
}

void Game::reserveSocket()
{
    _servFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_servFd == -1)
        error(1, errno, "socket failed");
}

void Game::setReuseAddr(int sock)
{
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (res)
        error(1, errno, "setsockopt failed");
}

void Game::setReuseAddr()
{
    setReuseAddr(_servFd);
}

int Game::getSocket()
{
    return _servFd;
}

void Game::sendToAll(int fd, char *buffer, int count)
{
    auto it = clients.begin();
    while (it != clients.end())
    {
        Client *client = *it;
        it++;
        if (client->fd() != fd)
            client->TriggerClientEvent(buffer); // TODO do poprawy
    }
}

void Game::setPort(char *txt)
{
    char *ptr;
    auto port = strtol(txt, &ptr, 10);
    if (*ptr != 0 || port < 1 || (port > ((1 << 16) - 1)))
        error(1, 0, "illegal argument %s", txt);
    _port = port;
}

void Game::getReadyForConnection()
{
    sockaddr_in serverAddr{.sin_family = AF_INET, .sin_port = htons((short)_port), .sin_addr = {INADDR_ANY}};
    int res = bind(_servFd, (sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res)
        error(1, errno, "bind failed");

    res = listen(_servFd, 1);
    if (res)
        error(1, errno, "listen failed");
}

void Game::prepareServer()
{
    reserveSocket();
    setReuseAddr();
    getReadyForConnection();
}

void Game::closeServer()
{
    for (Client *client : clients)
        delete client;
    close(_servFd);
    printf("Closing server\n");
    exit(0);
}