

#include "connection.hpp"
#include "game.hpp"

#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <poll.h>
#include <iostream>
#include <signal.h>
// #include <cstdlib>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <thread>
Game game;

void ctrl_c(int elo)
{
    game.closeClient();
}

int main(int argc, char **argv)
{

    if (argc == 3)
    {
        game.setServerAddress(argv[1], argv[2]);
    }
    else if (argc != 1)
    {
        error(1, 0, "Need 2 args");
    }

    signal(SIGINT, ctrl_c); // Przechwycenie ctrl+c
    signal(SIGPIPE, SIG_IGN);

    // Resolve arguments to IPv4 address with a port number
    game.serverConnect();

    int epollFd = epoll_create1(0);
    game.setEpollFd(epollFd);
    epoll_event epollevent;

    epollevent.events = EPOLLIN;
    epollevent.data.u64 = 692137420;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &epollevent);
    epollevent.data.ptr = &game;
    // epollevent.data.u64 = game.getSocket();
    epoll_ctl(epollFd, EPOLL_CTL_ADD, game.getSocket(), &epollevent);

    while (true)
    {
        epoll_wait(epollFd, &epollevent, 1, -1);
        // std::cout << epollevent.data.ptr << " | " << &game << " | " << epollevent.data.u64 << " Event:" << epollevent.events << std::endl;

        if (epollevent.events & EPOLLIN && epollevent.data.u64 == 692137420)
        {
            // read from stdin, write to socket
            ssize_t bufsize = 255, received;
            char buffer[bufsize];
            received = read(0, buffer, bufsize);
            buffer[received - 1] = '\0';
            received -= 1;
            if (received <= 0)
            {
                std::cout << "Nic nie zostało wpisane" << std::endl;
            }
            std::string s_buffer = std::string(buffer);
            game.handleInput(s_buffer);
        }
        else
        {
            ((Handler *)epollevent.data.ptr)->handleEvent(epollevent.events);
        }
    }

    return 0;
}
