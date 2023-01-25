#include "game.hpp"

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>
#include <iostream>

Game game;
int epollFd;

void ctrl_c(int elo)
{
    game.safeCloseServer();
}

class ServHandler : Handler
{
public:
    virtual void handleEvent(uint32_t events, int _fd) override
    {
        if (events & EPOLLIN)
        {
            sockaddr_in clientAddr{};
            socklen_t clientAddrSize = sizeof(clientAddr);

            auto clientFd = accept(game.getSocket(), (sockaddr *)&clientAddr, &clientAddrSize);
            if (clientFd == -1)
                error(1, errno, "accept failed");

            printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);

            game.newClient(clientFd);

            epoll_event ee{EPOLLIN | EPOLLRDHUP, {.u64 = static_cast<uint64_t>(clientFd)}};
            epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ee);
        }
        if (events & ~EPOLLIN)
        {
            error(0, errno, "Event %x on server socket", events);
            ctrl_c(SIGINT);
        }
    }
};

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        error(1, 0, "Need 1 arg (port)");
    } // TODO dodać default port

    signal(SIGINT, ctrl_c); // Przechwycenie ctrl+c
    signal(SIGPIPE, SIG_IGN);

    game.setPort(argv[1]);
    game.prepareServer();

    epollFd = epoll_create1(0);
    game.setEpollFd(epollFd);
    ServHandler servHandler;
    epoll_event ee{EPOLLIN, {.u64 = 0}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, game.getSocket(), &ee);

    while (true)
    {
        if (-1 == epoll_wait(epollFd, &ee, 1, -1))
        {
            error(0, errno, "epoll_wait failed");
            ctrl_c(SIGINT);
        }
        // std::cout << &game << " " << ee.data.ptr << " | " << ee.data.u64 << " Event:" << ee.events << " fd:" << ee.data.u64 << std::endl;
        if (ee.data.u64 == 0)
        {
            servHandler.handleEvent(ee.events, -1);
        }
        else
        {
            game.handleEvent(ee.events, ee.data.u64);
        }
    }
}
