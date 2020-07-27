#include "Mirroring.h"

#include <stdio.h>
#include <stdlib.h>

static Mirroring* pMirroring = NULL;

bool MirroringCallback(void* pMirroringPacket) {
    ONYPACKET_UINT8* pPacket = (ONYPACKET_UINT8*)pMirroringPacket;

    int iDataLen = ntohl( *(u_int32_t*)&pPacket[1] );
    short usCmd = ntohs( *(short*)&pPacket[5] );
    int nHpNo = *(&pPacket[7]);
    if(nHpNo < 1 || nHpNo > MAXCHCNT) {
        return false;
    }

    if(usCmd == CMD_MIRRORING_JPEG_CAPTURE_FAILED) {
        pMirroring->HandleJpegCaptureFailedPacket(pPacket, iDataLen, usCmd, nHpNo);
    } else {
        pMirroring->HandleJpegPacket(pPacket, iDataLen, usCmd, nHpNo);
    }

    return false;
}

void MirrorStoppedCallback(int nHpNo, int nStopCode) {
    pMirroring->OnMirrorStopped(nHpNo, nStopCode);
}

Mirroring::Mirroring() {
    for(int i = 0; i < MAXCHCNT; i++) {
        m_pMirroringRoutine[i] = NULL;
        m_pMirroringStopRoutine[i] = NULL;

        m_nDeviceOrientation[i] = 1;
    }

    pMirroring = this;
}

Mirroring::~Mirroring() {

}

bool Mirroring::StartMirroring(int nHpNo, 
            PMIRRORING_ROUTINE pMirroringRoutine, 
            PMIRRORING_STOP_ROUTINE pMirroringStopRoutine) {
    bool ret = false;
    int nMirrorPort = 8800 + nHpNo;
    int nControlPort = 8820 + nHpNo;

    if(m_pMirroringStopRoutine[nHpNo - 1] == NULL) {
        // 쓰래드 시작
        if( m_mirClient[nHpNo - 1].StartRunClientThread(nHpNo, nMirrorPort, nControlPort, MirroringCallback, MirrorStoppedCallback) ) {
            m_pMirroringRoutine[nHpNo - 1] = pMirroringRoutine;
            m_pMirroringStopRoutine[nHpNo - 1] = pMirroringStopRoutine;
            m_nDeviceOrientation[nHpNo -1] = 1;

            ret = true;
        }
    }

    printf("Start Mirroring : %s\n", ret ? "true" : "false");

    return ret;
}

void Mirroring::StopMirroring(int nHpNo) {
    if(m_pMirroringStopRoutine[nHpNo - 1] != NULL) {
        // 쓰래드 종료
        m_mirClient[nHpNo - 1].StopRunClientThread();
    }
}

void Mirroring::HandleJpegPacket(ONYPACKET_UINT8* pPacket, int iDataLen, short usCmd, int nHpNo) {
    if(m_pMirroringRoutine[nHpNo - 1] != NULL) {
        m_pMirroringRoutine[nHpNo - 1](pPacket);
    }
}

void Mirroring::HandleJpegCaptureFailedPacket(ONYPACKET_UINT8* pPacket, int iDataLen, short usCmd, int nHpNo) {

}

void Mirroring::OnMirrorStopped(int nHpNo, int nStopCode) {
    if(m_pMirroringStopRoutine[nHpNo - 1] != NULL) {
        m_pMirroringRoutine[nHpNo - 1] = NULL;

        m_pMirroringStopRoutine[nHpNo - 1](nHpNo, nStopCode);
        m_pMirroringStopRoutine[nHpNo - 1] = NULL;
    }
}