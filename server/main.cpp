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

#include "connection.hpp"
#include "game.hpp"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        error(1, 0, "Need 1 arg (port)");
    } // TODO dodać default port

    auto port = readPort(argv[1]);

    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
        error(1, errno, "socket failed");

    signal(SIGINT, ctrl_c); // Przechwycenie ctrl+c
    signal(SIGPIPE, SIG_IGN);

    setReuseAddr(servFd);

    sockaddr_in serverAddr{.sin_family = AF_INET, .sin_port = htons((short)port), .sin_addr = {INADDR_ANY}};
    int res = bind(servFd, (sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res)
        error(1, errno, "bind failed");

    res = listen(servFd, 1);
    if (res)
        error(1, errno, "listen failed");

    epollFd = epoll_create1(0);

    epoll_event ee{EPOLLIN, {.ptr = &servHandler}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, servFd, &ee);

    while (true)
    {
        if (-1 == epoll_wait(epollFd, &ee, 1, -1))
        {
            error(0, errno, "epoll_wait failed");
            ctrl_c(SIGINT);
        }
        std::cout << ee.data.ptr << " | " << ee.data.u64 << " Event:" << ee.events << std::endl;
        ((Handler *)ee.data.ptr)->handleEvent(ee.events);
    }
}
