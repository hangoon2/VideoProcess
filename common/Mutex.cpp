#include "Mutex.h"

Mutex::Mutex() {
    pthread_mutex_init(&m_mutex, NULL);
}

Mutex::~Mutex() {

}

void Mutex::Lock() {
    pthread_mutex_lock(&m_mutex);
}

void Mutex::Unlock() {
    pthread_mutex_unlock(&m_mutex);
}