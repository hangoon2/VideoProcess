#include "MIR_Queue.h"

#include <arpa/inet.h>

MIR_Queue::MIR_Queue() {

}

MIR_Queue::~MIR_Queue() {
    ClearQueueInternal();
}

void MIR_Queue::EnQueue(BYTE* item) {
//    m_mQueueLock.lock();

    BYTE* itemClone = CreateQueueItem(item);
    if(itemClone != nullptr) {
        m_mirQueue.push_back(itemClone);
    }

//    m_mQueueLock.unlock();
}

bool MIR_Queue::DeQueue(BYTE* o_item) {
    bool ret = false;

//    m_mQueueLock.lock();

    if(m_mirQueue.size() > 0) {
        mir_que_t::iterator iterBegin = m_mirQueue.begin();
        BYTE* item = *iterBegin;
        m_mirQueue.pop_front();

        memcpy(o_item, item, MIR_DEFAULT_MEM_POOL_UINT);

        DeleteQueueItem(item);

        ret = true;
    }

//    m_mQueueLock.unlock();

    return ret;
}

void MIR_Queue::ClearQueue() {
//    m_mQueueLock.lock();

    ClearQueueInternal();

//    m_mQueueLock.unlock();
}

void* MIR_Queue::AllocateItemMemory() {
    return m_memPool.Alloc();
}

void MIR_Queue::FreeItemMemory(void* pMemory) {
    m_memPool.Free(pMemory);
}

BYTE* MIR_Queue::CreateQueueItem(BYTE* pSrc) {
    BYTE* ret = NULL;

    BYTE* pPacket = pSrc;

    UINT iDataLen = ntohl( *(UINT*)&pPacket[1] );

    UINT nTotLen = CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE;
    if(nTotLen > MIR_DEFAULT_MEM_POOL_UINT) {
        return NULL;
    }

    ret = (BYTE*)AllocateItemMemory();

    if(ret != NULL) {
        memcpy(ret, pSrc, nTotLen);
    }

    return ret;
}

void MIR_Queue::DeleteQueueItem(BYTE* item) {
    if(item != NULL) {
        FreeItemMemory(item);
    }
}

void MIR_Queue::ClearQueueInternal() {
    mir_que_t::iterator iterBegin = m_mirQueue.begin();
    mir_que_t::iterator iterEnd = m_mirQueue.end();
    BYTE* item = NULL;

    for(mir_que_t::iterator iterPos = iterBegin; iterPos != iterEnd; ++iterPos) {
        item = *iterPos;
        DeleteQueueItem(item);
    }

    m_mirQueue.clear();
}