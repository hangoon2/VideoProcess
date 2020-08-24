#ifndef MIR_MEMPOOL_H
#define MIR_MEMPOOL_H

#include "../common/VPSCommon.h"
#include "../common/Mutex.h"

class MIR_MemPool {
public:
    MIR_MemPool();
    virtual ~MIR_MemPool();

    void* Alloc();
    void Free(void* pMem);

    int Size();

private:
    BYTE* m_pMemPool;
    bool m_isAllocedFlag[MIR_DEFAULT_MEM_POOL_UNIT_COUNT];

    Mutex m_mMemPool;

    int m_nAllocedCount;
};

#endif 
