#pragma once
#include <thread>

class Timer
{
    void countdown();
    int _seconds;
    bool _stop;
    std::thread _timerThread;

public:
    Timer();
    int getTimeLeft() const;
    void startTimer(int seconds);
    void stopTimer();
    virtual void timerDone();
    virtual void secondPassed();
    ~Timer();
};