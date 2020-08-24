#include "Callback_Queue.h"

Callback_Queue::Callback_Queue() {

}

Callback_Queue::~Callback_Queue() {
    ClearQueueInternal();
}

void Callback_Queue::EnQueue(HDCAP* recInfo) {
//    m_mQueueLock.lock();

    HDCAP* itemClone = CreateQueueItem(recInfo);
    if(itemClone != nullptr) {
        m_recQueue.push_back(itemClone);
    }

//    m_mQueueLock.unlock();
}

bool Callback_Queue::DeQueue(HDCAP* o_recInfo) {
    bool ret = false;

//    m_mQueueLock.lock();

    if(m_recQueue.size() > 0) {
        rec_que_t::iterator iterBegin = m_recQueue.begin();
        HDCAP* item = *iterBegin;
        m_recQueue.pop_front();

        memcpy(o_recInfo, item, sizeof(HDCAP));

        DeleteQueueItem(item);

        ret = true;
    }

//    m_mQueueLock.unlock();

    return ret;
}

void Callback_Queue::ClearQueue() {
//    m_mQueueLock.lock();

    ClearQueueInternal();

//    m_mQueueLock.unlock();
}

void* Callback_Queue::AllocateItemMemory() {
    return m_memPool.Alloc();
}

void Callback_Queue::FreeItemMemory(void* pMemory) {
    m_memPool.Free(pMemory);
}

HDCAP* Callback_Queue::CreateQueueItem(HDCAP* pSrc) {
    HDCAP* ret = nullptr;

    ret = (HDCAP*)AllocateItemMemory();

    if(ret != nullptr) {
        memcpy(ret, pSrc, sizeof(HDCAP));
    }

    return ret;
}

void Callback_Queue::DeleteQueueItem(HDCAP* item) {
    if(item != nullptr) {
        FreeItemMemory(item);
    }
}

void Callback_Queue::ClearQueueInternal() {
    rec_que_t::iterator iterBegin = m_recQueue.begin();
    rec_que_t::iterator iterEnd = m_recQueue.end();
    HDCAP* item = NULL;

    for(rec_que_t::iterator iterPos = iterBegin; iterPos != iterEnd; ++iterPos) {
        item = *iterPos;
        DeleteQueueItem(item);
    }

    m_recQueue.clear();
}
