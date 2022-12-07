#include "game.hpp"
#include "connection.hpp"

#include <iostream>
#include <sys/epoll.h> //epoll
#include <unistd.h>    //read

void Game::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        char buffer[256] = "";
        ssize_t count = read(getSocket(), buffer, 256); // TODO dowiedzieć się jak to wyciągnąć z
        if (count > 0)
        { // TODO deserializacja
            std::string eventName = buffer;
            if (doesEventExist(eventName))
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
                if (doesEventExist(eventName))
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
        // TODO
    }
}
