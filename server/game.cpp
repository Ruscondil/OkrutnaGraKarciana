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
            { // TODO dodać error
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
    std::string buffer;
    serializeEventName(buffer, arg);
    std::cout << "SIZE1 " << buffer.size() << std::endl;
    serializeInt(buffer, 2137);
    serializeString(buffer, "floppa friday i soggota");
    std::cout << "SIZE2 " << buffer.size() << std::endl;
    printText(buffer);
    // std::cout << "CZEMU ZAWSZE SA PROBLEMY" << std::endl;

    sendToAll(buffer);
}