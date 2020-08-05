#include "Rec_MemPool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Rec_MemPool::Rec_MemPool() {
    m_pHdCap = (HDCAP*)malloc(sizeof(HDCAP) * REC_DEFAULT_MEM_POOL_UNIT_COUNT);

    for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; i++) {
        memset(&m_pHdCap[i], 0, sizeof(HDCAP));
        m_isAllocedFlag[i] = false;
    }

    m_nAllocedCount = 0;
}

Rec_MemPool::~Rec_MemPool() {

}

void* Rec_MemPool::Alloc() {
    void* ret = nullptr;

//    m_mRecMemLock.lock();

    // 미리 할당된 메모리가 사용 전이면, 미리 할당된 메모리를 사용하고,
    // 그렇지 않으면, 새로운 메모리를 할당
    if(m_nAllocedCount < REC_DEFAULT_MEM_POOL_UNIT_COUNT) {
        for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(m_isAllocedFlag[i] == false) {
                ret = (void*)&m_pHdCap[i];
                m_isAllocedFlag[i] = true;

                ++m_nAllocedCount;
                break;
            }
        }
    } else {
        printf("Recording New Memory Alloc : %d\n", m_nAllocedCount);
        ret = malloc(sizeof(HDCAP));
    }

//    m_mRecMemLock.unlock();

    return ret;
}

void Rec_MemPool::Free(void *pMem) {
    if(pMem == nullptr) {
        return;
    }

//    m_mRecMemLock.lock();

    bool isFreed = false;

    if(m_nAllocedCount > 0) {
        for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(pMem == (void*)&m_pHdCap[i]) {
                m_isAllocedFlag[i] = false;
                --m_nAllocedCount;

                isFreed = true;
                break;
            }
        }
    }

    if(!isFreed) {
        free(pMem);
    }

//    m_mRecMemLock.unlock();
}
