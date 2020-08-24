#ifndef MIR_QUEUE_HANDLER_H
#define MIR_QUEUE_HANDLER_H

#include "../common/VPSCommon.h"
#include "MIR_Queue.h"

#include <pthread.h>

class MIR_QueueHandler {
public:
    MIR_QueueHandler();
    virtual ~MIR_QueueHandler();

    bool StartThread(PMIRRORING_ROUTINE fnProcessPacket);
    void StopThread();

    void QueueHandling();
    void EnQueue(BYTE* item);

private:
    PMIRRORING_ROUTINE m_fnProcessPacket;

    MIR_Queue m_mirQue;

    BYTE m_tempQueueItem[MIR_DEFAULT_MEM_POOL_UINT];

    pthread_t m_tID;

    bool m_isThreadRunning;
};

#endif