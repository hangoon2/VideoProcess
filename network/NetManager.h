#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "AsyncMediaServerSocket.h"

class NetManager {
public:
    NetManager();
    ~NetManager();

    void OnServerModeStart();
    bool OnReadEx(ClientObject* pObject, char* pData, int len);

private:
    bool WebCommandDataParsing2(ClientObject* pObject, char* pData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;
};

#endif
