#include "Mirroring.h"
#include "VPSJpeg.h"
#include "MIR_QueueHandler.h"

#include <stdio.h>
#include <stdlib.h>

static Mirroring* gs_pMirroring = NULL;

static MIR_QueueHandler gs_mirQueHandler[MAXCHCNT];
static VPSJpeg gs_mirJpeg[MAXCHCNT];

static int MIRROR_PORT = 8800;
static int CONTROL_PORT = 8820;

bool MirroringEnQueue(void* pMirroringPacket) {
    BYTE* pPacket = (BYTE*)pMirroringPacket;

    int nHpNo = *(&pPacket[7]);

    gs_pMirroring->EnQueue(nHpNo, pPacket);

    return true;
}

bool MirroringCallback(void* pMirroringPacket) {
    BYTE* pPacket = (BYTE*)pMirroringPacket;

    int iDataLen = ntohl( *(u_int32_t*)&pPacket[1] );
    short usCmd = ntohs( *(short*)&pPacket[5] );
    int nHpNo = *(&pPacket[7]);
    if(nHpNo < 1 || nHpNo > MAXCHCNT) {
        return false;
    }

    if(usCmd == CMD_MIRRORING_CAPTURE_FAILED) {
        gs_pMirroring->HandleJpegCaptureFailedPacket(pPacket, nHpNo);
    } else {
        gs_pMirroring->HandleJpegPacket(pPacket, iDataLen, usCmd, nHpNo);
    }

    return true;
}

void MirrorStoppedCallback(int nHpNo, int nStopCode) {
    gs_pMirroring->OnMirrorStopped(nHpNo, nStopCode);
}

Mirroring::Mirroring() {
    for(int i = 0; i < MAXCHCNT; i++) {
        m_pMirroringRoutine[i] = NULL;
        m_pMirroringStopRoutine[i] = NULL;

        m_nDeviceOrientation[i] = 1;
        m_nDeviceDefaultOrientation[i] = 1;

        m_nJpgQuality[i] = VPS_DEFAULT_JPG_QUALITY;

        m_nKeyFrameW[i] = 0;
        m_nKeyFrameH[i] = 0;

#if ENABLE_MIRRORING_QUEUE
        gs_mirQueHandler[i].StartThread(MirroringCallback);
#endif
    }

    char value[16] = {0,};
    GetPrivateProfileString(VPS_SZ_SECTION_STREAM, VPS_SZ_KEY_MIRROR_PORT, "", value);
    MIRROR_PORT = atoi(value);

    GetPrivateProfileString(VPS_SZ_SECTION_STREAM, VPS_SZ_KEY_CONTROL_PORT, "", value);
    CONTROL_PORT = atoi(value);

    gs_pMirroring = this;
}

Mirroring::~Mirroring() {
#if ENABLE_MIRRORING_QUEUE
    for(int i = 0; i < MAXCHCNT; i++) {
        gs_mirQueHandler[i].StopThread();
    }
#endif
}

void Mirroring::Initialize(PMIRRORING_ROUTINE pRecordRoutine, PVPS_ADD_LOG_ROUTINE pLogRoutine) {
    m_pRecordingRoutine = pRecordRoutine;
    m_pLogRoutine = pLogRoutine;

    for(int i = 0; i < MAXCHCNT; i++) {
        m_mirClient[i].SetAddLogLisnter(pLogRoutine);
    }
}

bool Mirroring::StartMirroring(int nHpNo, int width, int height, 
            PMIRRORING_ROUTINE pMirroringRoutine, 
            PMIRRORING_STOP_ROUTINE pMirroringStopRoutine) {
    bool ret = false;
    int nMirrorPort = MIRROR_PORT + nHpNo;
    int nControlPort = CONTROL_PORT + nHpNo;

    if(m_pMirroringStopRoutine[nHpNo - 1] == NULL) {
        if(width > height) {
            m_nDeviceDefaultOrientation[nHpNo - 1] = 0;
        } else {
            m_nDeviceDefaultOrientation[nHpNo - 1] = 1;
        }

        // 쓰래드 시작
#if ENABLE_MIRRORING_QUEUE
        if( m_mirClient[nHpNo - 1].StartRunClientThread(nHpNo, nMirrorPort, nControlPort, MirroringEnQueue, MirrorStoppedCallback) ) {
#else
        if( m_mirClient[nHpNo - 1].StartRunClientThread(nHpNo, nMirrorPort, nControlPort, MirroringCallback, MirrorStoppedCallback) ) {
#endif
            m_pMirroringRoutine[nHpNo - 1] = pMirroringRoutine;
            m_pMirroringStopRoutine[nHpNo - 1] = pMirroringStopRoutine;
            m_nDeviceOrientation[nHpNo -1] = 1;

            ret = true;
        }
    }

    return ret;
}

void Mirroring::StopMirroring(int nHpNo) {
    if(m_pMirroringStopRoutine[nHpNo - 1] != NULL) {
        // 쓰래드 종료
        m_mirClient[nHpNo - 1].StopRunClientThread();
    }
}

void Mirroring::HandleJpegPacket(BYTE* pPacket, int iDataLen, short usCmd, int nHpNo) {
    short nLeft = ntohs( *(short*)&pPacket[16] );
    short nTop = ntohs( *(short*)&pPacket[18] );
    short nRight = ntohs( *(short*)&pPacket[20] );
    short nBottom = ntohs( *(short*)&pPacket[22] );
    bool isKeyFrame = *&pPacket[24] == 1 ? true : false;

    if(isKeyFrame) {
        m_nKeyFrameW[nHpNo - 1] = nRight;
        m_nKeyFrameH[nHpNo - 1] = nBottom;
    }

    int nKeyFrameWidth = m_nKeyFrameW[nHpNo - 1];
    int nKeyFrameHeight = m_nKeyFrameH[nHpNo - 1];
    int nLongerKeyFrameLength = nKeyFrameWidth > nKeyFrameHeight ? nKeyFrameWidth : nKeyFrameHeight;
    int nShorterKeyFrameLength = nKeyFrameWidth < nKeyFrameHeight ? nKeyFrameWidth : nKeyFrameHeight;

    // Portrait인 기본 디바이스가 화면 화면 상태가 Landscape 상태로 종료될 경우
    // nxptc_vd가 실행될때 디바이스의 기본모드를 Landscape로 잘못 설정한 경우 
    // CMD_JPG_LANDSCAPE, CMD_JPG_PORTRAIT을 보낸다. 반대의 경우도 발생
    if(m_nDeviceDefaultOrientation[nHpNo - 1] == 1) {
        if(usCmd == CMD_JPG_LANDSCAPE) {
            usCmd = CMD_JPG_DEV_VERT_IMG_VERT;
            *(short*)&pPacket[5] = htons(usCmd);
        } else if(usCmd == CMD_JPG_PORTRAIT) {
            usCmd = CMD_JPG_DEV_HORI_IMG_HORI;
            *(short*)&pPacket[5] = htons(usCmd);
        }
    } else {
        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT) {
            usCmd = CMD_JPG_LANDSCAPE;
            *(short*)&pPacket[5] = htons(usCmd);
        } else if(usCmd == CMD_JPG_DEV_HORI_IMG_HORI) {
            usCmd = CMD_JPG_PORTRAIT;
            *(short*)&pPacket[5] = htons(usCmd);
        }
    }

    if(usCmd == CMD_JPG_LANDSCAPE || usCmd == CMD_JPG_PORTRAIT) {
        // 디바이스 회전을 고려한 command code 변경: 1=세로, 0=가로
        int nDeviceOrientation = m_nDeviceOrientation[nHpNo - 1];
        if(usCmd == CMD_JPG_LANDSCAPE) {
            if(nDeviceOrientation == 1) {
                usCmd = CMD_JPG_DEV_VERT_IMG_HORI;
            } else {
                usCmd = CMD_JPG_DEV_HORI_IMG_HORI;
            }
        } else {
            if(nDeviceOrientation == 1) {
                usCmd = CMD_JPG_DEV_VERT_IMG_VERT;
            } else {
                usCmd = CMD_JPG_DEV_HORI_IMG_VERT;
            }
        }
        *(short*)&pPacket[5] = htons(usCmd);

        // JPEG 회전을 위한 코드
        BYTE* pJpgData = pPacket + 25;
        int nJpgDataSize = iDataLen - 17;

        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT || usCmd == CMD_JPG_DEV_VERT_IMG_HORI) {
            int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateRight( pJpgData, nJpgDataSize, m_nJpgQuality[nHpNo - 1], (nRight - nLeft), (nBottom - nTop) );
            if(nNewJpgDataSize > 0) {
                *(short*)&pPacket[16] = htons(nShorterKeyFrameLength - nBottom);
                *(short*)&pPacket[18] = htons(nLeft);
                *(short*)&pPacket[20] = htons(nShorterKeyFrameLength - nTop);
                *(short*)&pPacket[22] = htons(nRight);

                iDataLen = iDataLen - nJpgDataSize + nNewJpgDataSize;
                *(int*)&pPacket[1] = htonl(iDataLen);

                *(short*)&pPacket[CMD_HEAD_SIZE + iDataLen] = htons( CalChecksum((UINT16*)(pPacket + 1), iDataLen + CMD_HEAD_SIZE - 1) );
                *&pPacket[CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE - 1] = CMD_END_CODE;
            }
        }
    } else {
        // 가로가 더 긴 디바이스에 대한 대응
        // VPS는 무조건 가로가 짧고, 세로가 긴 디바이스를 가정하므로 이에 맞게 패킷 번호 변경
        bool isWideDevice = nKeyFrameWidth > nKeyFrameHeight;
        if(isWideDevice) {
            if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT) {
                usCmd = CMD_JPG_DEV_HORI_IMG_HORI;
            } else if(usCmd == CMD_JPG_DEV_HORI_IMG_HORI) {
                usCmd = CMD_JPG_DEV_VERT_IMG_VERT;
            }

            *(short*)&pPacket[5] = htons(usCmd);
        } 

        // 디바이스 회전을 고려한 command code 변경: 1=세로, 0=가로
        int nDeviceOrientation = m_nDeviceOrientation[nHpNo - 1];
        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT && nDeviceOrientation == 0) {
            usCmd = CMD_JPG_DEV_HORI_IMG_VERT;
            *(short*)&pPacket[5] = htons(usCmd);
        } else if(usCmd == CMD_JPG_DEV_HORI_IMG_HORI && nDeviceOrientation == 1) {
            usCmd = CMD_JPG_DEV_VERT_IMG_HORI;
            *(short*)&pPacket[5] = htons(usCmd);
        }

        // JPEG 회전을 위한 코드
        BYTE* pJpgData = pPacket + 25;
        int nJpgDataSize = iDataLen - 17;

        if( (usCmd == CMD_JPG_DEV_HORI_IMG_HORI || usCmd == CMD_JPG_DEV_HORI_IMG_VERT) && !isWideDevice ) {
            int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateLeft( pJpgData, nJpgDataSize, m_nJpgQuality[nHpNo - 1], (nRight - nLeft), (nBottom - nTop) );
            if(nNewJpgDataSize > 0) {
                *(short*)&pPacket[16] = htons(nTop);
                *(short*)&pPacket[18] = htons(nShorterKeyFrameLength - nRight);
                *(short*)&pPacket[20] = htons(nBottom);
                *(short*)&pPacket[22] = htons(nShorterKeyFrameLength - nLeft);

                iDataLen = iDataLen - nJpgDataSize + nNewJpgDataSize;
                *(int*)&pPacket[1] = htonl(iDataLen);

                *(short*)&pPacket[CMD_HEAD_SIZE + iDataLen] = htons( CalChecksum((UINT16*)(pPacket + 1), iDataLen + CMD_HEAD_SIZE - 1) );
                *&pPacket[CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE - 1] = CMD_END_CODE;
            }
        } else if( usCmd == CMD_JPG_DEV_VERT_IMG_VERT && isWideDevice ) {
            int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateLeft( pJpgData, nJpgDataSize, m_nJpgQuality[nHpNo - 1], (nRight - nLeft), (nBottom - nTop) );
            if(nNewJpgDataSize > 0) {
                *(short*)&pPacket[16] = htons(nTop);
                *(short*)&pPacket[18] = htons(nLongerKeyFrameLength - nRight);
                *(short*)&pPacket[20] = htons(nBottom);
                *(short*)&pPacket[22] = htons(nLongerKeyFrameLength - nLeft);

                iDataLen = iDataLen - nJpgDataSize + nNewJpgDataSize;
                *(int*)&pPacket[1] = htonl(iDataLen);

                *(short*)&pPacket[CMD_HEAD_SIZE + iDataLen] = htons( CalChecksum((UINT16*)(pPacket + 1), iDataLen + CMD_HEAD_SIZE - 1) );
                *&pPacket[CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE - 1] = CMD_END_CODE;
            }
        } else if( usCmd == CMD_JPG_DEV_VERT_IMG_HORI && isWideDevice ) {
            int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateRight( pJpgData, nJpgDataSize, m_nJpgQuality[nHpNo - 1], (nRight - nLeft), (nBottom - nTop) );
            if(nNewJpgDataSize > 0) {
                *(short*)&pPacket[16] = htons(nShorterKeyFrameLength - nBottom);
                *(short*)&pPacket[18] = htons(nLeft);
                *(short*)&pPacket[20] = htons(nShorterKeyFrameLength - nTop);
                *(short*)&pPacket[22] = htons(nRight);

                iDataLen = iDataLen - nJpgDataSize + nNewJpgDataSize;
                *(int*)&pPacket[1] = htonl(iDataLen);

                *(short*)&pPacket[CMD_HEAD_SIZE + iDataLen] = htons( CalChecksum((UINT16*)(pPacket + 1), iDataLen + CMD_HEAD_SIZE - 1) );
                *&pPacket[CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE - 1] = CMD_END_CODE;
            }
        }
    }

    if(m_pMirroringRoutine[nHpNo - 1] != NULL) {
        short l = ntohs( *(short*)&pPacket[16] );
        short t = ntohs( *(short*)&pPacket[18] );
        short r = ntohs( *(short*)&pPacket[20] );
        short b = ntohs( *(short*)&pPacket[22] );

        if(l < 0 || t < 0 || r < 0 || b < 0) return;

        m_pMirroringRoutine[nHpNo - 1](pPacket);
    }
}

void Mirroring::HandleJpegCaptureFailedPacket(BYTE* pPacket, int nHpNo) {
    if(m_pMirroringRoutine[nHpNo - 1] != NULL) {
        m_pMirroringRoutine[nHpNo - 1](pPacket);
    }
}

void Mirroring::OnMirrorStopped(int nHpNo, int nStopCode) {
    if(m_pMirroringStopRoutine[nHpNo - 1] != NULL) {
        m_pMirroringRoutine[nHpNo - 1] = NULL;

        m_pMirroringStopRoutine[nHpNo - 1](nHpNo, nStopCode);
        m_pMirroringStopRoutine[nHpNo - 1] = NULL;
    }
}

void Mirroring::SendKeyFrame(int nHpNo) {
    if(m_pMirroringStopRoutine[nHpNo - 1] == NULL) return;

    m_mirClient[nHpNo - 1].SendKeyFramePacket();
}

void Mirroring::SendControlPacket(int nHpNo, BYTE* pData, int len) {
    if(m_pMirroringStopRoutine[nHpNo - 1] == NULL) return;
    
    short usCmd = ntohs( *(short*)&pData[5] );
    if(usCmd == CMD_PLAYER_QUALITY) {
        int quality = ntohs( *(short*)&pData[8] );

        if(quality < 1 || quality > 100) {
            printf("[VPS:%d] Quality 값이 범위를 벗어남: %d\n",  nHpNo, quality);
            return;
        }

        m_nJpgQuality[nHpNo - 1] = quality;
    }

    m_mirClient[nHpNo - 1].SendToControlSocket((const char*)pData, len);
}   

void Mirroring::SetDeviceOrientation(int nHpNo, int deviceOrientation) {
    m_nDeviceOrientation[nHpNo - 1] = deviceOrientation;
}

void Mirroring::EnQueue(int nHpNo, BYTE* pPacket) {
    gs_mirQueHandler[nHpNo - 1].EnQueue(pPacket);
}