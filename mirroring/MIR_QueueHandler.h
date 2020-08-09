#ifndef MIR_QUEUE_HANDLER_H
#define MIR_QUEUE_HANDLER_H

#include "../common/VPSCommon.h"
#include "MIR_Queue.h"

#include <pthread.h>

typedef void (*PROCESS_PACKET_FN)(BYTE* pPacket);

class MIR_QueueHandler {
public:
    MIR_QueueHandler();
    virtual ~MIR_QueueHandler();

    bool StartThread(PROCESS_PACKET_FN fnProcessPacket);

private:
    PROCESS_PACKET_FN m_fnProcessPacket;

    MIR_Queue m_mirQue;

    BYTE m_tempQueueItem[MIR_DEFAULT_MEM_POOL_UINT];

    pthread_t m_tID;

    bool m_isThreadReady;
    bool m_doExitThread;
    bool m_isThreadRunning;
};

#endif