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

    void ClientConnected(ClientObject* pClient);
    void ClientDisconnected(ClientObject* pClient);

    void Record(int nHpNo, bool start);
    
    void CleanupClient(int nHpNo);
    bool ClosingClient(ClientObject* pClient);

    void StopVideoServiceIfNoClientExist();
    bool CloseClientManualEx(int nHpNo);
    void CloseClientManual(ClientObject* pClient);

private:
    bool SendToMobileController(BYTE* pData, int iLen, bool force = false);
    bool SendToClient(short usCmd, int nHpNo, BYTE* pData, int iLen, int iKeyFrameNo);
    bool Send(BYTE* pData, int iLen, ClientObject* pClient, bool force);

    bool JPGCaptureAndSend(int nHpNo, BYTE* pJpgData, int iJpgDataLen);
    bool JPGCaptureFailSend(int nHpNo);
    bool RecordStopAndSend(int nHpNo);
    bool RecordStopSend(int nHpNo);
    bool SendLastRecordStopResponse(int nHpNo);
    bool DoMirrorVideoRecording(int nHpNo, short usCmd, bool isKeyFrame, BYTE* pPacket, int iDataLen);

    bool HeartbeatSendToDC();
    bool VideoFpsCheckAndSend(void* pData);

    bool WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len);

private:
    AsyncMediaServerSocket m_VPSSvr;

    Mirroring m_mirror;

    int m_serverPort;

    char m_strAviSavePath[256];

    bool m_isOnService[MAXCHCNT];

    bool m_isJpgCapture[MAXCHCNT];
    ULONGLONG m_nJpgCaptureStartTime[MAXCHCNT];

    bool m_isRunRecord[MAXCHCNT];
    ULONGLONG m_nRecStartTime[MAXCHCNT];

    bool m_isReceivedRecordStartCommand[MAXCHCNT];
    int m_nRecordStartCommandCountReceived[MAXCHCNT];
    int m_nRecordStopCommandCountReceived[MAXCHCNT];
    int m_nRecordCompletionCountSent[MAXCHCNT];
    
    int m_nCaptureCommandReceivedCount[MAXCHCNT];
    int m_nCaptureCompletionCountSent[MAXCHCNT];

    int m_iMirrorVideoInputCount[MAXCHCNT];

    UINT m_iCaptureCount[MAXCHCNT];
    UINT m_iCaptureCountOld[MAXCHCNT];
    UINT m_iCompressedCount[MAXCHCNT];
    UINT m_iCompressedCountOld[MAXCHCNT];
    UINT m_iSendCount[MAXCHCNT];
    UINT m_iSendCountOld[MAXCHCNT];

    int m_iRefreshCH[MAXCHCNT];

    bool m_isToSkipFailureReport[MAXCHCNT];

    BYTE m_byOldVideoFpsStatus[MAXCHCNT];

    Timer m_timer_1;
    Timer m_timer_10;
    Timer m_timer_20;
};

#endif
