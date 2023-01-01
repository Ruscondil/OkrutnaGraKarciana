#include "game.hpp"
#include "connection.hpp"
#include "handler.hpp"

#include <iostream>
#include <sys/epoll.h> //epoll
#include <unistd.h>    //read
#include <functional>  //std::bind
#include <cstring>     //memcpy
void Game::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        char buffer[256] = "";
        ssize_t count = read(getSocket(), buffer, 256); // TODO dowiedzieć się jak to wyciągnąć z

        std::string s_buffer;

        // Resize the string to fit the buffer
        s_buffer.resize(count);

        // Copy the buffer data to the string
        memcpy(&s_buffer[0], buffer, count);

        if (count > 0)
        { // TODO deserializacja
            std::string eventName = getEventName(s_buffer);
            std::string arguments = s_buffer;
            std::cout << s_buffer << std::endl;
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
    // registerNetEvent('receiveLeadboard');
    // registerNetEvent('newRound');
    // registerNetEvent('receiveFinishRoundInfo');
}

void Game::test(std::string a)
{
    int test;
    test = deserializeInt(a);
    std::cout << a << " | " << test << std::endl;
}