#pragma once
#include <thread>

class Timer
{
    void countdown();
    int _seconds;
    std::thread _timerThread;

public:
    Timer();
    int getTimeLeft() const;
    void startTimer(int seconds);
    virtual void timerDone();
    virtual void secondPassed();
    ~Timer();
};