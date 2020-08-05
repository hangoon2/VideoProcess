#ifndef REC_QUEUE_H
#define REC_QUEUE_H

#include "Rec_MemPool.h"

#include <deque>

using namespace std;

typedef deque<HDCAP*> rec_que_t;

class Rec_Queue {
public:
    Rec_Queue();
    virtual ~Rec_Queue();

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
    Rec_MemPool m_memPool;

//    QMutex m_mQueueLock;
};

#endif
