#ifndef MIRRORING_H
#define MIRRORING_H

#include "MIR_Client.h"

class Mirroring {
public:
    Mirroring();
    virtual ~Mirroring();

    bool StartMirroring(int nHpNo, PMIRRORING_ROUTINE pMirroringRoutine, PMIRRORING_STOP_ROUTINE pMirroringStopRoutine);
    void StopMirroring(int nHpNo);

    void HandleJpegPacket(ONYPACKET_UINT8* pPacket, int iDataLen, short usCmd, int nHpNo);
    void HandleJpegCaptureFailedPacket(ONYPACKET_UINT8* pPacket, int iDataLen, short usCmd, int nHpNo);
    void OnMirrorStopped(int nHpNo, int nStopCode);

private:
    PMIRRORING_ROUTINE m_pMirroringRoutine[MAXCHCNT];
    PMIRRORING_STOP_ROUTINE m_pMirroringStopRoutine[MAXCHCNT];

    bool m_nDeviceOrientation[MAXCHCNT];

    MIR_Client m_mirClient[MAXCHCNT];
};

#endif