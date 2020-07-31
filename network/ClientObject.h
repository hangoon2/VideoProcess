#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include "../common/VPSCommon.h"

#include <pthread.h>

#define RECV_BUFFER_SIZE        1024 * 1024
#define DEFAULT_STRING_SIZE     128

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

    char m_strID[DEFAULT_STRING_SIZE];
    char m_strIPAddr[DEFAULT_STRING_SIZE];

    int m_nHpNo;
    int m_nClientType;

    bool m_isExitCommandReceived;

    bool m_isFirstImage;

    void Lock();
    void Unlock();

    const char* GetClientTypeString();

private:
    pthread_mutex_t m_lock;
};

#endif