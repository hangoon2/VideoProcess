#ifndef REC_MEMPOOL_H
#define REC_MEMPOOL_H

#include "../common/VPSCommon.h"

#define REC_DEFAULT_MEM_POOL_UNIT_COUNT 20

class Rec_MemPool {
public:
    Rec_MemPool();
    virtual ~Rec_MemPool();

    void* Alloc();
    void Free(void* pMem);

private:
    HDCAP* m_pHdCap;
    bool m_isAllocedFlag[REC_DEFAULT_MEM_POOL_UNIT_COUNT];

//    QMutex m_mRecMemLock;

    int m_nAllocedCount;
};

#endif 
