#ifndef MIR_MEMPOOL_H
#define MIR_MEMPOOL_H

#include "../common/VPSCommon.h"

#define MIR_DEFAULT_MEM_POOL_UINT   (1024 * 100)
#define MIR_DEFAULT_MEM_POOL_UNIT_COUNT 20

class MIR_MemPool {
public:
    MIR_MemPool();
    virtual ~MIR_MemPool();

    void* Alloc();
    void Free(void* pMem);

private:
    BYTE* m_pMemPool;
    bool m_isAllocedFlag[MIR_DEFAULT_MEM_POOL_UNIT_COUNT];

//    QMutex m_mRecMemLock;

    int m_nAllocedCount;
};

#endif 
