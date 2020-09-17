#include "Callback_Queue.h"

Callback_Queue::Callback_Queue() {

}

Callback_Queue::~Callback_Queue() {
    ClearQueueInternal();
}

void Callback_Queue::EnQueue(CALLBACK* cbInfo) {
    m_mQueueLock.Lock();

    CALLBACK* itemClone = CreateQueueItem(cbInfo);
    if(itemClone != NULL) {
        m_cbQueue.push_back(itemClone);
    }

    m_mQueueLock.Unlock();
}

bool Callback_Queue::DeQueue(CALLBACK* o_cbInfo) {
    bool ret = false;

    m_mQueueLock.Lock();

    if(m_cbQueue.size() > 0) {
        cb_que_t::iterator iterBegin = m_cbQueue.begin();
        CALLBACK* item = *iterBegin;
        m_cbQueue.pop_front();

        memcpy(o_cbInfo, item, sizeof(CALLBACK));

        DeleteQueueItem(item);

        ret = true;
    }

    m_mQueueLock.Unlock();

    return ret;
}

void Callback_Queue::ClearQueue() {
    m_mQueueLock.Lock();

    ClearQueueInternal();

    m_mQueueLock.Unlock();
}

void* Callback_Queue::AllocateItemMemory() {
    return m_memPool.Alloc();
}

void Callback_Queue::FreeItemMemory(void* pMemory) {
    m_memPool.Free(pMemory);
}

CALLBACK* Callback_Queue::CreateQueueItem(CALLBACK* pSrc) {
    CALLBACK* ret = NULL;

    ret = (CALLBACK*)AllocateItemMemory();

    if(ret != NULL) {
        memcpy(ret, pSrc, sizeof(CALLBACK));
    }

    return ret;
}

void Callback_Queue::DeleteQueueItem(CALLBACK* item) {
    if(item != nullptr) {
        FreeItemMemory(item);
    }
}

void Callback_Queue::ClearQueueInternal() {
    cb_que_t::iterator iterBegin = m_cbQueue.begin();
    cb_que_t::iterator iterEnd = m_cbQueue.end();
    CALLBACK* item = NULL;

    for(cb_que_t::iterator iterPos = iterBegin; iterPos != iterEnd; ++iterPos) {
        item = *iterPos;
        DeleteQueueItem(item);
    }

    m_cbQueue.clear();
}
