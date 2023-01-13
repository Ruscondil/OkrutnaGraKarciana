#pragma once
#include <thread>

class Timer
{
    void countdown();
    int _seconds;
    std::thread _timerThread;

public:
    Timer();
    Timer(int seconds);
    int getTimeLeft() const;
    void startTimer();
    virtual void timerDone();
    virtual void secondPassed();
    ~Timer();
};