#include "ClientObject.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

ClientObject::ClientObject() {
    m_rcvCommandBuffer = new char[RECV_BUFFER_SIZE];
    m_rcvPos = 0;

    memset(m_rcvCommandBuffer, 0x00, RECV_BUFFER_SIZE);

    memset( m_strID, 0x00, sizeof(m_strID) );
    memset( m_strIPAddr, 0x00, sizeof(m_strIPAddr) );

    m_nHpNo = 0;
    m_nClientType = CLIENT_TYPE_UNKNOWN;

    m_isExitCommandReceived = false;

    m_isFirstImage = true;

    m_isClosing = false;
}

ClientObject::~ClientObject() {
    Lock();

    // printf("[VPS:%d] Socket %d closed\n", m_nHpNo, m_clientSock);
    // close(m_clientSock);

    m_clientSock = INVALID_SOCKET;
    
    delete [] m_rcvCommandBuffer;

    Unlock();
}

void ClientObject::Lock() {
    m_lock.Lock();
}

void ClientObject::Unlock() {
    m_lock.Unlock();
}

const char* ClientObject::GetClientTypeString() {
    switch(m_nClientType) {
        case CLIENT_TYPE_MC:
        return VPS_SZ_MOBILE_CONTROLLER;

        case CLIENT_TYPE_HOST:
        return VPS_SZ_CLIENT_HOST;

        case CLIENT_TYPE_GUEST:
        return VPS_SZ_CLIENT_GUEST;

        case CLIENT_TYPE_MONITOR:
        return VPS_SZ_CLIENT_MONITOR;
    }

    return VPS_SZ_CLIENT_UNKNOWN;
}