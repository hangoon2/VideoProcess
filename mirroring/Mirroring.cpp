#include "Mirroring.h"

Mirroring::Mirroring() {
    for(int i = 0; i < MAXCHCNT; i++) {
        m_pMirroringRoutine[i] = NULL;
        m_pMirroringStopRoutine[i] = NULL;

        m_nDeviceOrientation[i] = 1;
    }
}

Mirroring::~Mirroring() {

}

bool Mirroring::StartMirroring(int nHpNo, 
            PMIRRORING_ROUTINE pMirroringRoutine, 
            PMIRRORING_STOP_ROUTINE pMirroringStopRoutine) {
    bool ret = false;
    int nMirrorPort = 8801;
    int nControlPort = 8821;

    if(m_pMirroringStopRoutine[nHpNo - 1] == NULL) {
        // 쓰래드 시작
        
        m_pMirroringRoutine[nHpNo - 1] = pMirroringRoutine;
        m_pMirroringStopRoutine[nHpNo - 1] = pMirroringStopRoutine;
        m_nDeviceOrientation[nHpNo -1] = 1;
        
        ret = true;
    }

    return ret;
}

void Mirroring::StopMirroring(int nHpNo) {
    if(m_pMirroringStopRoutine[nHpNo - 1] != NULL) {
        // 쓰래드 종료
    }
}