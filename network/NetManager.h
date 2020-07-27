#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "AsyncMediaServerSocket.h"
#include "../mirroring/Mirroring.h"

class NetManager {
public:
    NetManager();
    ~NetManager();

    void OnServerModeStart();

    bool BypassPacket(void* pMirroringPacket);

    bool OnReadEx(ClientObject* pClient, char* pRcvData, int len);

private:
    bool SendToClient(short usCmd, int nHpNo, ONYPACKET_UINT8* pData, int iLen, int iKeyFrameNo);

    bool CloseClient(ClientObject* pClient);
    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    Mirroring m_mirror;

    bool m_isOnService[MAXCHCNT];
};

#endif
