#ifndef MIRRORING_H
#define MIRRORING_H

#include "MIR_Client.h"

class Mirroring {
public:
    Mirroring();
    virtual ~Mirroring();

    void Initialize(PMIRRORING_ROUTINE pRecordRoutine, PVPS_ADD_LOG_ROUTINE pLogRoutine);
    bool StartMirroring(int nHpNo, PMIRRORING_ROUTINE pMirroringRoutine, PMIRRORING_STOP_ROUTINE pMirroringStopRoutine);
    void StopMirroring(int nHpNo);

    void HandleJpegPacket(BYTE* pPacket, int iDataLen, short usCmd, int nHpNo);
    void HandleJpegCaptureFailedPacket(BYTE* pPacket, int nHpNo);
    void OnMirrorStopped(int nHpNo, int nStopCode);

    void SendKeyFrame(int nHpNo);
    void SendControlPacket(int nHpNo, BYTE* pData, int len);

    void SetDeviceOrientation(int nHpNo, int deviceOrientation);

    void EnQueue(int nHpNo, BYTE* pPacket);

private:
    PMIRRORING_ROUTINE m_pMirroringRoutine[MAXCHCNT];
    PMIRRORING_STOP_ROUTINE m_pMirroringStopRoutine[MAXCHCNT];
    PMIRRORING_ROUTINE m_pRecordingRoutine;
    PVPS_ADD_LOG_ROUTINE m_pLogRoutine;

    int m_nDeviceOrientation[MAXCHCNT];

    int m_nJpgQuality[MAXCHCNT];

    int m_nKeyFrameW[MAXCHCNT];
    int m_nKeyFrameH[MAXCHCNT];

    MIR_Client m_mirClient[MAXCHCNT];
};

#endif