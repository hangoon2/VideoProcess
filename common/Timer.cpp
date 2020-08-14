#include "Timer.h"

//#include <unistd.h>

Timer::Timer() {
    m_isExecute = false;
}

Timer::~Timer() {

}

void Timer::Start(int interval, function<void(int)> func) {
    m_isExecute = true;

    thread([=]() {
        while(m_isExecute) {
            this_thread::sleep_for( chrono::milliseconds(interval) );
//            usleep(interval * 1000);
            func(interval);
        }    
    }).detach();
}

void Timer::Stop() {
    m_isExecute = false;
}