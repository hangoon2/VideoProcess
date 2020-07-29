#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <thread>

using namespace std;

class Timer {
public:
    Timer();
    virtual ~Timer();

    void Start(int interval, function<void(int)> func);
    void Stop();

private:
    bool m_isExecute;
};

#endif