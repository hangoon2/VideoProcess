#include "Timer.h"

Timer::Timer() {
    m_isExecute = false;
}

Timer::~Timer() {

}

void Timer::Start(int interval, function<void(int)> func) {
    m_isExecute = true;

    thread([=]() {
        while(m_isExecute) {
            func(interval);
            this_thread::sleep_for( chrono::milliseconds(interval) );
        }    
    }).detach();
}

void Timer::Stop() {
    m_isExecute = false;
}