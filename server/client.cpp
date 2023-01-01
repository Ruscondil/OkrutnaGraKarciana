#include "client.hpp"
#include "handler.hpp"
// #include <cstdlib>
// #include <cstdio>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>
#include <iostream>

Client::Client(int fd) : _fd(fd)
{
    // epoll_event ee{EPOLLIN | EPOLLRDHUP, {.ptr = this}};
    // epoll_ctl(_epollFd, EPOLL_CTL_ADD, _fd, &ee); // TODO naprawić
    // registerNetEvent("a", &Client::Test);
}

Client::~Client() // destuktor
{
    // epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
}

int Client::fd() const { return _fd; }

void Client::changeNickname(std::string nickname)
{
    _nickname = nickname;
}

void Client::handleEvent(uint32_t events, int _fd)
{ /*
     if (events & EPOLLIN)
     {
         char buffer[256] = "";
         ssize_t count = read(_fd, buffer, 256);
         if (count > 0)
         { // TODO deserializacja
             std::string eventName = buffer;
             std::string arguments = "TESTARGUMENTSERV";
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
         _clientStatus = LOST;
     }*/
}

/* void Client::remove()
{
    printf("removing %d\n", _fd);
    clients.erase(this);
    delete this;
} */

void Client::TriggerClientEvent(std::string eventName) // TODO dodać argumenty
{
    std::cout << "TRIGGER" << std::endl;
    int count = eventName.length();
    // char buffer[] = "ELO"; // TODO dodać serializacje i argumenty
    if (count != ::write(_fd, (char *)eventName.c_str(), count))
    {
        _clientStatus = LOST;
    }
    // remove();
    // TODO zmienić status na niektywny
}

void Client::Test(std ::string eventName)
{
    std::cout << "TEST" << std::endl;
    // TriggerClientEvent(eventName);
}