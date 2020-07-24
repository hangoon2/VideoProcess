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
}

ClientObject::~ClientObject() {
    printf("Call ClientObject Destructor\n");
    
    delete [] m_rcvCommandBuffer;

    printf("Socket %d closed\n", m_clientSock);
    close(m_clientSock);
}