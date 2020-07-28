#ifndef MIR_CLIENT_H
#define MIR_CLIENT_H

#include "../common/VPSCommon.h"

#include <pthread.h>

typedef bool (*PMIRRORING_ROUTINE)(void* pMirroringPacket);
typedef void (*PMIRRORING_STOP_ROUTINE)(int nHpNo, int nStopCode);

#define SEND_BUF_SIZE   512
#define RECV_BUF_SIZE   (2 * 1024 * 1024)

enum {
    RX_PACKET_POS_START,
    RX_PACKET_POS_HEAD,
    RX_PACKET_POS_DATA,
    RX_PACKET_POS_TAIL
};

class MIR_Client {
public:
    MIR_Client();
    virtual ~MIR_Client();

    bool StartRunClientThread(int nHpNo, int nMirroringPort, int nControlPort, 
                                PMIRRORING_ROUTINE pMirroringRoutine, PMIRRORING_STOP_ROUTINE pMirroringStopRoutine);
    void StopRunClientThread();

    void OnMirrorStopped(int nStopCode);

    void SetIsThreadRunning(bool isThreadRunning);
    bool IsThreadRunning();
    void SetDoExitRunClientThread(bool doExitRunClientThread);
    bool IsDoExitRunClientThread();
    void SetRunClientThreadReady(bool isRunClientThreadReady);
    bool IsRunClientThreadReady();

    void CleanUpRunClientThreadData();

    int GetHpNo();
    int GetMirrorPort();
    int GetControlPort();

    int SendOnOffPacket(bool onoff);
    int SendKeyFramePacket(int nHpNo);

    bool GetData(int efd, Socket sock, int waitms);

private:
    int SendToControlSocket(const char* buf, int len);

    ONYPACKET_UINT8* MakeOnyPacketKeyFrame(int nHpNo, int& size);
    ONYPACKET_UINT8* MakeOnyPacketOnOff(int nHpNo, bool onoff, int& size);

public:
    int m_mirrorSocket;
    int m_controlSocket;

    ONYPACKET_UINT8* m_pRcvBuf;

private:
    PMIRRORING_ROUTINE m_pMirroringRoutine;
    PMIRRORING_STOP_ROUTINE m_pMirroringStopRoutine;

    pthread_t m_tID;

    bool m_isThreadRunning;
    bool m_doExitRunClientThread;
    bool m_isRunClientThreadReady;

    int m_nHpNo;
    int m_nMirrorPort;
    int m_nControlPort;

    // int m_nOffset;
    // int m_nCurrReadSize;
    // int m_nReadBufSize;
    // bool m_isHeadOfFrame;
    // bool m_isFirstImage;

    ONYPACKET_UINT8 m_sendBuf[SEND_BUF_SIZE];

    int m_pos;
    //int m_iWrite;
    int m_rxStreamOrder;
    int m_dataSize;
};

#endif