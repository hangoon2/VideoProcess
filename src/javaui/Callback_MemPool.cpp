#include "Callback_MemPool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Callback_MemPool::Callback_MemPool() {
    m_pCallback = (CALLBACK*)malloc(sizeof(CALLBACK) * REC_DEFAULT_MEM_POOL_UNIT_COUNT);

    for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; i++) {
        memset(&m_pCallback[i], 0, sizeof(CALLBACK));
        m_isAllocedFlag[i] = false;
    }

    m_nAllocedCount = 0;
}

Callback_MemPool::~Callback_MemPool() {
    m_mRecMemLock.Lock();

    if(m_pCallback) {
        free(m_pCallback);
    }

    m_mRecMemLock.Unlock();
}

void* Callback_MemPool::Alloc() {
    void* ret = nullptr;

    m_mRecMemLock.Lock();

    // 미리 할당된 메모리가 사용 전이면, 미리 할당된 메모리를 사용하고,
    // 그렇지 않으면, 새로운 메모리를 할당
    if(m_nAllocedCount < REC_DEFAULT_MEM_POOL_UNIT_COUNT) {
        for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(m_isAllocedFlag[i] == false) {
                ret = (void*)&m_pCallback[i];
                m_isAllocedFlag[i] = true;

                ++m_nAllocedCount;
                break;
            }
        }
    } else {
        printf("Callback New Memory Alloc : %d\n", m_nAllocedCount);
        ret = malloc(sizeof(CALLBACK));
    }

    m_mRecMemLock.Unlock();

    return ret;
}

void Callback_MemPool::Free(void *pMem) {
    if(pMem == nullptr) {
        return;
    }

    m_mRecMemLock.Lock();

    bool isFreed = false;

    if(m_nAllocedCount > 0) {
        for(int i = 0; i < REC_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(pMem == (void*)&m_pCallback[i]) {
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

    m_mRecMemLock.Unlock();
}
