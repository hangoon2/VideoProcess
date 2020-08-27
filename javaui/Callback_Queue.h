#ifndef CALLBACK_QUEUE_H
#define CALLBACK_QUEUE_H

#include "Callback_MemPool.h"
#include "../common/Mutex.h"

#include <deque>

using namespace std;

typedef deque<CALLBACK*> cb_que_t;

class Callback_Queue {
public:
    Callback_Queue();
    virtual ~Callback_Queue();

    void EnQueue(CALLBACK* cbInfo);
    bool DeQueue(CALLBACK* o_cbInfo);
    void ClearQueue();

private:
    void* AllocateItemMemory();
    void FreeItemMemory(void* pMemory);
    CALLBACK* CreateQueueItem(CALLBACK* pSrc);
    void DeleteQueueItem(CALLBACK* item);
    void ClearQueueInternal();

private:
    cb_que_t m_cbQueue;
    Callback_MemPool m_memPool;

    Mutex m_mQueueLock;
};

#endif
