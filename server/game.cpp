#include "game.hpp"
#include <errno.h>
#include <error.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <functional>
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
        std::string s_buffer = std::string(buffer);
        if (count > 0)
        { // TODO deserializacja
            std::string eventName = getEventName(s_buffer);
            std::string arguments = "TEST";

            EventFunction callback = getNetEventCallback(eventName);
            if (callback)
            {
                callback(arguments);
            }
            else
            { // TODO sprawdzanie globalnej mapy
                std::cout << "Wrong event, bit sus: " << count << " " << eventName << std::endl;
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

void Game::sendToAll(std::string buffer)
{
    auto it = clients.begin();
    while (it != clients.end())
    {
        Client *client = *it;
        it++;
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
    registerNetEvent("testowanie", std::bind(&Game::test, this, std::placeholders::_1));
}
// TODO powywalać rzeczy do z sieci do osobnego pliku
void Game::closeServer()
{
    for (Client *client : clients)
        delete client;
    close(_servFd);
    printf("Closing server\n");
    exit(0);
}

void Game::test(std::string arg)
{
    std::cout << "SIZE1 " << arg.size() << std::endl;
    serializeInt(arg, 2137);
    std::cout << "SIZE2 " << arg.size() << std::endl;
    // std::cout << "CZEMU ZAWSZE SA PROBLEMY" << std::endl;

    sendToAll(arg);
}