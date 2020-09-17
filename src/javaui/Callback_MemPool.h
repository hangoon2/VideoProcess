#ifndef CALLBACK_MEMPOOL_H
#define CALLBACK_MEMPOOL_H

#include "../common/VPSCommon.h"
#include "../common/Mutex.h"

#define REC_DEFAULT_MEM_POOL_UNIT_COUNT 30

class Callback_MemPool {
public:
    Callback_MemPool();
    virtual ~Callback_MemPool();

    void* Alloc();
    void Free(void* pMem);

private:
    CALLBACK* m_pCallback;
    bool m_isAllocedFlag[REC_DEFAULT_MEM_POOL_UNIT_COUNT];

    int m_nAllocedCount;

    Mutex m_mRecMemLock;
};

#endif 
