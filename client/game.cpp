#include "game.hpp"
#include "connection.hpp"
#include "handler.hpp"

#include <iostream>
#include <sys/epoll.h> //epoll
#include <unistd.h>    //read
#include <functional>  //std::bind
#include <cstring>     //memcpy

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
        std::cout << "Count: " << count << std::endl;
        std::string s_buffer;

        // Resize the string to fit the buffer
        s_buffer.resize(count);
        // Copy the buffer data to the string
        memcpy(&s_buffer[0], buffer, count);
        if (count > 0)
        {
            // printText(s_buffer);
            std::string eventName = getEventName(s_buffer);
            std::string arguments = s_buffer;
            // printText(s_buffer);
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
    registerNetEvent("beginClientConnection", std::bind(&Game::startConnection, this, std::placeholders::_1));
    // registerNetEvent('receiveLeadboard');
    // registerNetEvent('newRound');
    // registerNetEvent('receiveFinishRoundInfo');
}

void Game::test(std::string a)
{
    int test;
    std::string test2;
    // printText(a);
    test = deserializeInt(a);
    test2 = deserializeString(a);
    std::cout << test << " | " << test2 << std::endl;
}

void Game::startConnection(std::string buffer)
{
    int clientServerFd = deserializeInt(buffer);
    std::cout << clientServerFd << std::endl;
}
