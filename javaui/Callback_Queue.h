#ifndef CALLBACK_QUEUE_H
#define CALLBACK_QUEUE_H

#include "Callback_MemPool.h"

#include <deque>

using namespace std;

typedef deque<HDCAP*> rec_que_t;

class Callback_Queue {
public:
    Callback_Queue();
    virtual ~Callback_Queue();

    void EnQueue(HDCAP* recInfo);
    bool DeQueue(HDCAP* o_recInfo);
    void ClearQueue();

private:
    void* AllocateItemMemory();
    void FreeItemMemory(void* pMemory);
    HDCAP* CreateQueueItem(HDCAP* pSrc);
    void DeleteQueueItem(HDCAP* item);
    void ClearQueueInternal();

private:
    rec_que_t m_recQueue;
    Callback_MemPool m_memPool;

//    QMutex m_mQueueLock;
};

#endif
