#include "MIR_MemPool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MIR_MemPool::MIR_MemPool() {
    m_pMemPool = (BYTE*)malloc(MIR_DEFAULT_MEM_POOL_UINT * MIR_DEFAULT_MEM_POOL_UNIT_COUNT);

    for(int i = 0; i < MIR_DEFAULT_MEM_POOL_UNIT_COUNT; i++) {
        m_isAllocedFlag[i] = false;
    }

    m_nAllocedCount = 0;
}

MIR_MemPool::~MIR_MemPool() {
    m_mMemPool.Lock();

    if(m_pMemPool) {
        free(m_pMemPool);
    }

    m_mMemPool.Unlock();
}

int MIR_MemPool::Size() {
    return m_nAllocedCount;
}

void* MIR_MemPool::Alloc() {
    void* ret = NULL;

    m_mMemPool.Lock();

    // 미리 할당된 메모리가 사용 전이면, 미리 할당된 메모리를 사용하고,
    // 그렇지 않으면, 새로운 메모리를 할당
    if(m_nAllocedCount < MIR_DEFAULT_MEM_POOL_UNIT_COUNT) {
        for(int i = 0; i < MIR_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(m_isAllocedFlag[i] == false) {
                ret = (void*)&m_pMemPool[MIR_DEFAULT_MEM_POOL_UINT * i];
                m_isAllocedFlag[i] = true;

                ++m_nAllocedCount;
                break;
            }
        }
    } else {
        printf("Recording New Memory Alloc : %d\n", m_nAllocedCount);
        ret = malloc(MIR_DEFAULT_MEM_POOL_UINT);
    }

    m_mMemPool.Unlock();

    return ret;
}

void MIR_MemPool::Free(void *pMem) {
    if(pMem == NULL) {
        return;
    }

    m_mMemPool.Lock();

    bool isFreed = false;

    if(m_nAllocedCount > 0) {
        for(int i = 0; i < MIR_DEFAULT_MEM_POOL_UNIT_COUNT; ++i) {
            if(pMem == (void*)&m_pMemPool[MIR_DEFAULT_MEM_POOL_UINT * i]) {
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

    m_mMemPool.Unlock();
}
