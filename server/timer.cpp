#include "timer.hpp"

#include <chrono>
#include <error.h>

Timer::Timer() : _stop(false), _seconds(0), _timerThread() {}

int Timer::getTimeLeft() const
{
    return _seconds;
}

void Timer::startTimer(int seconds)
{

    if (_timerThread.joinable())
    {
        error(0, 0, "Timer jest już odpalony");
        return;
    }
    _stop = false;
    _seconds = seconds;
    _timerThread = std::thread(&Timer::countdown, this);
}

void Timer::stopTimer()
{
    _stop = true;
}

void Timer::countdown()
{
    auto start = std::chrono::system_clock::now();
    while (_seconds > 0 && !_stop)
    {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = now - start;
        if (elapsed.count() >= 1)
        {
            secondPassed(_seconds);
            _seconds--;
            start = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!_stop)
        timerDone();
}

void Timer::timerDone()
{
}

void Timer::secondPassed(int seconds)
{
}

void Timer::destroyTimer()
{
    if (_timerThread.joinable())
    {
        _timerThread.join();
    }
}

Timer::~Timer()
{
    destroyTimer();
}