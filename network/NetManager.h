#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "AsyncMediaServerSocket.h"

class NetManager {
public:
    NetManager();
    ~NetManager();

    void OnServerModeStart();
    bool OnReadEx(ClientObject* pClient, char* pRcvData, int len);

private:
    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    bool m_isOnService[MAXCHCNT];
};

#endif
