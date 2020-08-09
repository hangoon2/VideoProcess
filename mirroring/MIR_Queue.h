#ifndef MIR_QUEUE_H
#define MIR_QUEUE_H

#include "MIR_MemPool.h"

#include <deque>

using namespace std;

typedef deque<BYTE*> mir_que_t;

class MIR_Queue {
public:
    MIR_Queue();
    virtual ~MIR_Queue();

    void EnQueue(BYTE* item);
    bool DeQueue(BYTE* o_item);
    void ClearQueue();

private:
    void* AllocateItemMemory();
    void FreeItemMemory(void* pMemory);
    BYTE* CreateQueueItem(BYTE* pSrc);
    void DeleteQueueItem(BYTE* item);
    void ClearQueueInternal();

private:
    mir_que_t m_mirQueue;
    MIR_MemPool m_memPool;

//    QMutex m_mQueueLock;
};

#endif
