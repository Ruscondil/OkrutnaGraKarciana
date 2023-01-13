#include "timer.hpp"

#include <chrono>
#include <error.h>

Timer::Timer() : _timerThread() {}
Timer::Timer(int seconds) : _seconds(seconds), _timerThread() {}

int Timer::getTimeLeft() const
{
    return _seconds;
}

void Timer::startTimer()
{
    if (_timerThread.joinable())
    {
        error(0, 0, "Timer jest już odpalony");
        return;
    }
    _timerThread = std::thread(&Timer::countdown, this);
}

void Timer::countdown()
{
    auto start = std::chrono::system_clock::now();
    while (_seconds > 0)
    {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        if (elapsed.count() >= 1)
        {
            _seconds--;
            start = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    timerDone();
}

void Timer::timerDone()
{
}

void Timer::secondPassed()
{
}

Timer::~Timer()
{
    if (_timerThread.joinable())
    {
        _timerThread.join();
    }
}