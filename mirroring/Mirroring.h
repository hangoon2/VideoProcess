#ifndef MIRRORING_H
#define MIRRORING_H

#include "../common/VPSCommon.h"
#include "MIR_Client.h"

typedef bool (*PMIRRORING_ROUTINE)(void* pMirroringPacket);
typedef void (*PMIRRORING_STOP_ROUTINE)(int nHpNo, int nStopCode);

class Mirroring {
public:
    Mirroring();
    virtual ~Mirroring();

    bool StartMirroring(int nHpNo, PMIRRORING_ROUTINE pMirroringRoutine, PMIRRORING_STOP_ROUTINE pMirroringStopRoutine);
    void StopMirroring(int nHpNo);

private:
    PMIRRORING_ROUTINE m_pMirroringRoutine[MAXCHCNT];
    PMIRRORING_STOP_ROUTINE m_pMirroringStopRoutine[MAXCHCNT];

    bool m_nDeviceOrientation[MAXCHCNT];

    MIR_Client m_mirClient[MAXCHCNT];
};

#endif