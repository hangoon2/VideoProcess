#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include "../common/VPSCommon.h"

#define RECV_BUFFER_SIZE    1024 * 1024

enum {
    CLIENT_TYPE_UNKNOWN,
    CLIENT_TYPE_MC,
    CLIENT_TYPE_HOST,
    CLIENT_TYPE_GUEST,
    CLIENT_TYPE_MONITOR,
    CLIENT_TYPE_AUDIO
};

class ClientObject {
public:
    ClientObject();
    virtual ~ClientObject();

    Socket m_clientSock;
    char* m_rcvCommandBuffer;
    int m_rcvPos;

    char m_strID[64];
    char m_strIPAddr[64];
};

#endif