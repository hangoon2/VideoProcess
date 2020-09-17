#include "Rec_Queue.h"

Rec_Queue::Rec_Queue() {

}

Rec_Queue::~Rec_Queue() {
    ClearQueueInternal();
}

void Rec_Queue::EnQueue(HDCAP* recInfo) {
    m_mQueueLock.Lock();

    HDCAP* itemClone = CreateQueueItem(recInfo);
    if(itemClone != NULL) {
        m_recQueue.push_back(itemClone);
    }

    m_mQueueLock.Unlock();
}

bool Rec_Queue::DeQueue(HDCAP* o_recInfo) {
    bool ret = false;

    m_mQueueLock.Lock();

    if(m_recQueue.size() > 0) {
        rec_que_t::iterator iterBegin = m_recQueue.begin();
        HDCAP* item = *iterBegin;
        m_recQueue.pop_front();

        memcpy(o_recInfo, item, sizeof(HDCAP));

        DeleteQueueItem(item);

        ret = true;
    }

    m_mQueueLock.Unlock();

    return ret;
}

void Rec_Queue::ClearQueue() {
    m_mQueueLock.Lock();

    ClearQueueInternal();

    m_mQueueLock.Unlock();
}

void* Rec_Queue::AllocateItemMemory() {
    return m_memPool.Alloc();
}

void Rec_Queue::FreeItemMemory(void* pMemory) {
    m_memPool.Free(pMemory);
}

HDCAP* Rec_Queue::CreateQueueItem(HDCAP* pSrc) {
    HDCAP* ret = NULL;

    ret = (HDCAP*)AllocateItemMemory();

    if(ret != NULL) {
        memcpy(ret, pSrc, sizeof(HDCAP));
    }

    return ret;
}

void Rec_Queue::DeleteQueueItem(HDCAP* item) {
    if(item != NULL) {
        FreeItemMemory(item);
    }
}

void Rec_Queue::ClearQueueInternal() {
    rec_que_t::iterator iterBegin = m_recQueue.begin();
    rec_que_t::iterator iterEnd = m_recQueue.end();
    HDCAP* item = NULL;

    for(rec_que_t::iterator iterPos = iterBegin; iterPos != iterEnd; ++iterPos) {
        item = *iterPos;
        DeleteQueueItem(item);
    }

    m_recQueue.clear();
}
