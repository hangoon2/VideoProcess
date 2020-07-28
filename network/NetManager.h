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
    void OnMirrorStopped(int nHpNo, int nStopCode);

    bool OnReadEx(ClientObject* pClient, char* pRcvData, int len);

    bool IsOnService(int nHpNo);

private:
    bool SendToClient(short usCmd, int nHpNo, ONYPACKET_UINT8* pData, int iLen, int iKeyFrameNo);
    bool Send(int nHpNo, ONYPACKET_UINT8* pData, int iLen, ClientObject* pClient, bool force);

    bool CloseClient(ClientObject* pClient);
    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    Mirroring m_mirror;

    bool m_isOnService[MAXCHCNT];

    int m_iRefreshCH[MAXCHCNT];
};

#endif
