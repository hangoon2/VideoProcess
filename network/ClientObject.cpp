#include "ClientObject.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

ClientObject::ClientObject() {
    printf("Call ClientObject Constructor\n");

    m_rcvCommandBuffer = new char[RECV_BUFFER_SIZE];
    m_rcvPos = 0;

    memset(m_rcvCommandBuffer, 0x00, RECV_BUFFER_SIZE);

    memset( m_strID, 0x00, sizeof(m_strID) );
    memset( m_strIPAddr, 0x00, sizeof(m_strIPAddr) );

    m_nHpNo = 0;
    m_nClientType = CLIENT_TYPE_UNKNOWN;

    m_isExitCommandReceived = false;

    m_isFirstImage = true;

    pthread_mutex_init(&m_lock, NULL);
}

ClientObject::~ClientObject() {
    printf("Call ClientObject Destructor\n");
    printf("Socket %d closed\n", m_clientSock);
    close(m_clientSock);

    m_clientSock = INVALID_SOCKET;
    
    delete [] m_rcvCommandBuffer;
}

void ClientObject::Lock() {
    pthread_mutex_lock(&m_lock);
}

void ClientObject::Unlock() {
    pthread_mutex_unlock(&m_lock);
}