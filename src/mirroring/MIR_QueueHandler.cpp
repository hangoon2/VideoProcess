#include "MIR_QueueHandler.h"

#include <thread>

using namespace std;

void* HandlingThreadFunc(void* pArg) {
    MIR_QueueHandler* pHandler = (MIR_QueueHandler*)pArg;
    if(pHandler != NULL) {
        pHandler->QueueHandling();
    }

    return NULL;
}

MIR_QueueHandler::MIR_QueueHandler() {
    m_isThreadRunning = false;
}

MIR_QueueHandler::~MIR_QueueHandler() {
    
}

bool MIR_QueueHandler::StartThread(PMIRRORING_ROUTINE fnProcessPacket) {
    m_fnProcessPacket = fnProcessPacket;

    pthread_create(&m_tID, NULL, &HandlingThreadFunc, this);
    if(m_tID == NULL) {
        printf("[VPS:0] Mirroring thread creation fail[%d]\n", errno);
        return false;
    }

    return true;
}

void MIR_QueueHandler::StopThread() {
    m_isThreadRunning = false;

    m_tID = NULL;
}

void MIR_QueueHandler::QueueHandling() {
    m_isThreadRunning = true;

    while(m_isThreadRunning) {
        if( m_mirQue.DeQueue(m_tempQueueItem) ) {
            m_fnProcessPacket(m_tempQueueItem);
        }

        this_thread::sleep_for( chrono::milliseconds(1) );
    }

    m_mirQue.ClearQueue();
}

void MIR_QueueHandler::EnQueue(BYTE* item) {
    if(m_isThreadRunning) {
        m_mirQue.EnQueue(item);
    }
}