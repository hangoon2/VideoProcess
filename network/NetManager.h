#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "AsyncMediaServerSocket.h"
#include "../mirroring/Mirroring.h"
#include "../common/Timer.h"

class NetManager {
public:
    NetManager();
    ~NetManager();

    void OnServerModeStart();

    bool BypassPacket(void* pMirroringPacket);
    void OnMirrorStopped(int nHpNo, int nStopCode);

    bool OnReadEx(ClientObject* pClient, char* pRcvData, int len);

    bool IsOnService(int nHpNo);
    bool IsReceivedRecordStartCommand(int nHpNo);

    void UpdateState(int id);

private:
    bool SendToMobileController(ONYPACKET_UINT8* pData, int iLen, bool force = false);
    bool SendToClient(short usCmd, int nHpNo, ONYPACKET_UINT8* pData, int iLen, int iKeyFrameNo);
    bool Send(ONYPACKET_UINT8* pData, int iLen, ClientObject* pClient, bool force);

    bool CloseClient(ClientObject* pClient);
    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    Mirroring m_mirror;

    bool m_isOnService[MAXCHCNT];
    bool m_isReceivedRecordStartCommand[MAXCHCNT];
    bool m_isJpgCapture[MAXCHCNT];

    int m_nCaptureCommandReceivedCount[MAXCHCNT];

    int m_iRefreshCH[MAXCHCNT];

    Timer m_timer_1;
    Timer m_timer_10;
};

#endif
