#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <thread>
#include <sys/epoll.h>
#include <poll.h>

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
    if (argc != 3)
        error(1, 0, "Need 2 args");
    // Resolve arguments to IPv4 address with a port number
    addrinfo *resolved, hints = {.ai_flags = 0, .ai_family = AF_INET, .ai_socktype = SOCK_STREAM}; // TCP
    int res = getaddrinfo(argv[1], argv[2], &hints, &resolved);
    if (res || !resolved)
        error(1, 0, "getaddrinfo: %s", gai_strerror(res));

    // create socket
    int sock = socket(resolved->ai_family, resolved->ai_socktype, 0);
    if (sock == -1)
        error(1, errno, "socket failed");

    int epollFd = epoll_create1(0);

    epoll_event epollevent;
    epollevent.events = EPOLLIN;
    epollevent.data.u64 = 692137420;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &epollevent);
    epollevent.data.u64 = sock;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &epollevent);

    // attept to connect
    res = connect(sock, resolved->ai_addr, resolved->ai_addrlen);
    if (res)
        error(1, errno, "connect failed");

    // free memory
    freeaddrinfo(resolved);

    while (true)
    {
        int epollwait = epoll_wait(epollFd, &epollevent, 1, -1);
        if (epollevent.events & EPOLLIN && epollevent.data.u64 == 692137420)
        {
            // read from stdin, write to socket
            ssize_t bufsize = 255, received;
            char buffer[bufsize];
            received = readData(0, buffer, bufsize);
            if (received <= 0)
            {
                shutdown(sock, SHUT_RDWR);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, STDIN_FILENO, &epollevent);
                close(sock);
                exit(0);
            }
            writeData(sock, buffer, received);
        }
        else if (epollevent.events & EPOLLIN && epollevent.data.u64 == sock)
        {
            // read from socket, write to stdout
            ssize_t bufsize = 255, received;
            char buffer[bufsize];
            received = readData(sock, buffer, bufsize);
            if (received <= 0)
            {
                shutdown(sock, SHUT_RDWR);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, sock, &epollevent);
                close(sock);
                exit(0);
            }
            writeData(1, buffer, received);
        }
    }

    return 0;
}
