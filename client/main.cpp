
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <poll.h>
#include <iostream>

#include "connection.hpp"
#include "game.hpp"
// #include <cstdlib>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <thread>

ssize_t readData(int fd, char *buffer, ssize_t buffsize)
{
    auto ret = read(fd, buffer, buffsize);
    if (ret == -1)
        error(1, errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char *buffer, ssize_t count)
{
    auto ret = write(fd, buffer, count);
    if (ret == -1)
        error(1, errno, "write failed on descriptor %d", fd);
    if (ret != count)
        error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

int main(int argc, char **argv)
{

    Game game;
    if (argc == 3)
    {
        game.setServerAddress(argv[1], argv[2]);
    }
    else if (argc != 1)
    {
        error(1, 0, "Need 2 args");
    }

    // Resolve arguments to IPv4 address with a port number
    game.serverConnect();

    int epollFd = epoll_create1(0);
    epoll_event epollevent;

    epollevent.events = EPOLLIN;
    epollevent.data.u64 = 692137420;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &epollevent);
    epollevent.data.ptr = &game;
    // epollevent.data.u64 = game.getSocket();
    epoll_ctl(epollFd, EPOLL_CTL_ADD, game.getSocket(), &epollevent);

    while (true)
    {
        int epollwait = epoll_wait(epollFd, &epollevent, 1, -1);
        // std::cout << epollevent.data.ptr << " | " << epollevent.data.u64 << " Event:" << epollevent.events << std::endl;

        if (epollevent.events & EPOLLIN && epollevent.data.u64 == 692137420)
        {
            // read from stdin, write to socket
            ssize_t bufsize = 255, received;
            char buffer[bufsize];
            received = readData(0, buffer, bufsize);
            buffer[received - 1] = '\0'; // TODO usunąć, do testów
            received -= 1;
            if (received <= 0)
            {
                printf("Connection lost\n");
                shutdown(game.getSocket(), SHUT_RDWR);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, STDIN_FILENO, &epollevent);
                close(game.getSocket());
                exit(0);
            }
            writeData(game.getSocket(), buffer, received);
        }
        else
        {
            std::cout << "TEST" << std::endl;
            ((Handler *)epollevent.data.ptr)->handleEvent(epollevent.events);
        }
        /*  else if (epollevent.events & EPOLLIN && epollevent.data.u64 == game.getSocket())
         {
             // read from socket, write to stdout
             ssize_t bufsize = 255, received;
             char buffer[bufsize];
             received = readData(game.getSocket(), buffer, bufsize);

             if (received <= 0)
             {
                 printf("Connection lost\n");
                 shutdown(game.getSocket(), SHUT_RDWR);
                 epoll_ctl(epollFd, EPOLL_CTL_DEL, game.getSocket(), &epollevent);
                 close(game.getSocket());
                 exit(0);
             }
             writeData(1, buffer, received);
         } */
    }

    return 0;
}
