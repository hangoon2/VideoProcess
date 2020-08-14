#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

class Mutex {
public:
    Mutex();
    virtual ~Mutex();

    void Lock();
    void Unlock();

private:
    pthread_mutex_t m_mutex;
};

#endif