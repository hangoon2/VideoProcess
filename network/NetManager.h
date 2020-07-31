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

    void UpdateState(int id);

private:
    void ClientConnected(ClientObject* pClient);
    bool SendToMobileController(BYTE* pData, int iLen, bool force = false);
    bool SendToClient(short usCmd, int nHpNo, BYTE* pData, int iLen, int iKeyFrameNo);
    bool Send(BYTE* pData, int iLen, ClientObject* pClient, bool force);

    bool JPGCaptureAndSend(int nHpNo, BYTE* pJpgData, int iJpgDataLen);

    bool CloseClient(ClientObject* pClient);
    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    Mirroring m_mirror;

    char m_strAviSavePath[256];

    bool m_isOnService[MAXCHCNT];
    bool m_isJpgCapture[MAXCHCNT];

    int m_nRecordStartCommandCountReceived[MAXCHCNT];
    int m_nRecordStopCommandCountReceived[MAXCHCNT];
    int m_nCaptureCommandReceivedCount[MAXCHCNT];

    int m_iRefreshCH[MAXCHCNT];

    Timer m_timer_1;
    Timer m_timer_10;
};

#endif
