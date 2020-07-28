#include "NetManager.h"
#include "../common/VPSCommon.h"

#include <stdio.h>

static NetManager *pNetMgr = NULL;

uint16_t SwapEndianU2(uint16_t wValue) {
    return (uint16_t)((wValue >> 8) | (wValue << 8));
}

uint32_t SwapEndianU4(uint32_t nValue) {
    char bTmp;
    bTmp = *((char*)&nValue + 3);
    *((char*)&nValue + 3) = *((char*)&nValue + 0);
    *((char*)&nValue + 0) = bTmp;

    bTmp = *((char*)&nValue + 2);
    *((char*)&nValue + 2) = *((char*)&nValue + 1);
    *((char*)&nValue + 1) = bTmp;

    return nValue;
}

uint16_t in_checksum(short* ptr, int nbytes) {
    long sum = 0;
    short answer;
    short tmp;
    short sCodeBIG;

    while(nbytes >= 2) {
        // 읽어올 값이 남아있으면
        // 포인터 하나씩 증가하면서 sum값에 더함
        tmp = *ptr;
        sCodeBIG = SwapEndianU2( (short)tmp );
        sum += sCodeBIG;
        ptr++;

        nbytes -= 2;
    }

    if(nbytes == 1) {
        // 남은 읽어 올 값이 홀수이면 그것도 더해줌
        char tmp2 = *(char*)ptr;
        sCodeBIG = SwapEndianU2(tmp2);
        sum = sum + abs(tmp2);
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (short)~sum;
    answer = SwapEndianU2( (short)answer );

    return answer;
}

bool OnReadEx(ClientObject* pClient, char* pRcvData, int len) {
    if(pNetMgr != NULL)
        return pNetMgr->OnReadEx(pClient, pRcvData, len);

    return false;
}

bool DoMirrorCallback(void* pMirroringPacket) {
    if(pNetMgr != NULL) {
        pNetMgr->BypassPacket(pMirroringPacket);
    }

    return false;
}

void DoMirrorStoppedCallback(int nHpNo, int nStopCode) {
    if(pNetMgr != NULL) {
        pNetMgr->OnMirrorStopped(nHpNo, nStopCode);
    }
}

NetManager::NetManager() {
    printf("Call NetManager Constructor\n");

    for(int i = 0; i < MAXCHCNT; i++) {
        m_isOnService[i] = false;

        m_iRefreshCH[i] = 1;
    }

    pNetMgr = this;
}

NetManager::~NetManager() {
    printf("Call NetManager Destructor\n");
}

void NetManager::OnServerModeStart() {
    int server = m_VPSSvr.InitSocket(10001, ::OnReadEx);
    if(server <= 0) {
        printf("Media Server Socket 생성 실패\n");
    }
}

bool NetManager::BypassPacket(void* pMirroringPacket) {
    ONYPACKET_UINT8* pPacket = (ONYPACKET_UINT8*)pMirroringPacket;

    int iDataLen = ntohl( *(uint32_t*)&pPacket[1] );
    short usCmd = ntohs( *(short*)&pPacket[5] );
    int nHpNo = *(&pPacket[7]);
    bool isKeyFrame = *&pPacket[24] == 1 ? true : false;

    if(usCmd == CMD_MIRRORING_JPEG_CAPTURE_FAILED) {
        return SendToClient(usCmd, nHpNo, pPacket, CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE, 0);
    } else {
        int iKeyFrameNo = 0;
        // 다음 번에 KeyFrame을 보내라는 플래그가 설정되어 있으면
        if(m_iRefreshCH[nHpNo - 1] == 1) {
            if(isKeyFrame) {
                iKeyFrameNo = !m_iRefreshCH[nHpNo - 1];
                m_iRefreshCH[nHpNo - 1] = 0; 
            } else {
                m_mirror.SendKeyFrame(nHpNo);
                return false;
            }
        }

        if( SendToClient(usCmd, nHpNo, pPacket, CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE, iKeyFrameNo) ) {
            // 전송 count 업데이트
        } else {
            // 전송 실패 시 키프레임을 요청한다.
            m_mirror.SendKeyFrame(nHpNo); 
        }
    }

    return true;
}

void NetManager::OnMirrorStopped(int nHpNo, int nStopCode) {
    printf("MIRRONG THREAD[%d] STOPPED : %d\n", nHpNo, nStopCode);

    if( IsOnService(nHpNo) ) {
        // 아직 서비스 중인 상태에서 소켓이 닫힌 경우 DC에게 알린다.
    }
}

bool NetManager::OnReadEx(ClientObject* pClient, char* pRcvData, int len) {
//    printf("NetManager::OnReadEx Socket(%d) ----> %d\n", pClient->m_clientSock, len);
    if(pRcvData[0] == CMD_START_CODE && pClient->m_rcvCommandBuffer[0] != CMD_START_CODE) {
        memcpy(pClient->m_rcvCommandBuffer, pRcvData, len);
        pClient->m_rcvPos = len;
    } else {
        // 처음이 시작코드가 아니면 버린다. 
        // 항상 시작 코드부터 패킷을 만든다.
        if(pClient->m_rcvCommandBuffer[0] != CMD_START_CODE)
            return false;

        memcpy(&pClient->m_rcvCommandBuffer[pClient->m_rcvPos], pRcvData, len);
        pClient->m_rcvPos += len;
    }

    bool ret = false;

    while(1) {
        if(pClient->m_rcvCommandBuffer[0] != CMD_START_CODE) break;

        bool bPacketOK = false;
        int iDataLen = SwapEndianU4( *(int*)&pClient->m_rcvCommandBuffer[1] );
        printf("NetManager::OnReadEx Data Size : %d\n", iDataLen);
        int totDataLen = CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE;
        if(pClient->m_rcvPos >= totDataLen) {
            if(pClient->m_rcvCommandBuffer[totDataLen - 1]) {
                bPacketOK = true;
            }
        }

        // 패킷이 완성될때까지 반복
        if(bPacketOK == false) return false;

        WebCommandDataParsing2(pClient, pClient->m_rcvCommandBuffer, totDataLen);

        pClient->m_rcvCommandBuffer[0] = 0; // 처리 완료 플래그

        // 처리 후, 남은 패킷을 맨앞으로 당겨준다.
        int iRemain = pClient->m_rcvPos - totDataLen;
        if(iRemain > 0) {
            if(pClient->m_rcvCommandBuffer[totDataLen] == CMD_START_CODE) {
                memcpy(pClient->m_rcvCommandBuffer, &pClient->m_rcvCommandBuffer[totDataLen], iRemain);
                pClient->m_rcvPos = iRemain;
            } else {
                // 남은 것이 있는데, 첫 바이트가 CMD_START_CODE가 아니면 버린다.
            }
        }

        ret = true;
    }

    return ret;
}

bool NetManager::IsOnService(int nHpNo) {
    return m_isOnService[nHpNo - 1];
}

bool NetManager::SendToClient(short usCmd, int nHpNo, ONYPACKET_UINT8* pData, int iLen, int iKeyFrameNo) {
    int iClientCount = 0;
    ClientObject** clientList = m_VPSSvr.GetClientList(nHpNo);
    for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
        ClientObject* pClient = clientList[i];
        if(pClient == NULL) continue;
        
        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT 
            || usCmd == CMD_JPG_DEV_HORI_IMG_HORI 
            || usCmd == CMD_JPG_DEV_VERT_IMG_HORI 
            || usCmd == CMD_JPG_DEV_HORI_IMG_VERT) {
            // mirroring data
            if(pClient->m_isFirstImage) {
                if(iKeyFrameNo == 0) {
                    pClient->m_isFirstImage = false;
                } else {
                    continue;
                }
            }
        } else if(usCmd == CMD_LOGCAT 
            || usCmd == CMD_ACK 
            || usCmd == CMD_RESOURCE_USAGE_NETWORK 
            || usCmd == CMD_RESOURCE_USAGE_CPU 
            || usCmd == CMD_RESOURCE_USAGE_MEMORY) {
            if(pClient->m_nClientType != CLIENT_TYPE_HOST 
                && pClient->m_nClientType != CLIENT_TYPE_GUEST) {
                continue;
            }
        }

        if( Send(nHpNo, pData, iLen, pClient, true) ) {
            iClientCount++;
        }
    }

    return iClientCount > 0;
}

bool NetManager::Send(int nHpNo, ONYPACKET_UINT8* pData, int iLen, ClientObject* pClient, bool force) {
    if(pClient->m_clientSock == INVALID_SOCKET) return false;

    bool ret = false;

    pClient->Lock();

    ret = m_VPSSvr.OnSend(nHpNo, pClient->m_clientSock, pData, iLen, force);
    if(ret) {
        // client 데이터 전송량 업데이트
    }

    pClient->Unlock();

    return ret;
}

bool NetManager::CloseClient(ClientObject* pClient) {
    return m_VPSSvr.OnClose(pClient->m_clientSock);
}

bool NetManager::WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len) {
    int iDataLen = SwapEndianU4( *(int*)&pRcvData[1] );
    short usCmd = SwapEndianU2( *(short*)&pRcvData[5] );
    int nHpNo = (int)pRcvData[7];

    printf("Received Data : %d, %d, %d\n", nHpNo, usCmd, iDataLen);

    if(iDataLen > RECV_BUFFER_SIZE || len < CMD_HEAD_SIZE + CMD_TAIL_SIZE) {
        // 잘못된 길이의 패킷
        return false;
    }

    if(usCmd <= 0 || usCmd > 32767) {
        // unavailable command code
        return false;
    }

    if(nHpNo <= 0 || nHpNo > MAXCHCNT) {
        // unavailable device number
        return false;
    }

    uint16_t usChkRcv = *(uint16_t*)&pRcvData[CMD_HEAD_SIZE + iDataLen];
    uint16_t usChk = in_checksum( (short*)&pRcvData[1], 7 + iDataLen);
    if(usChkRcv != usChk) {
        printf("***** Wrong Check Sum  => Rcv: %X, Calc: %X\n", usChkRcv, usChk);
        return false;
    }

    bool ret = false;
    char* pData = &pRcvData[CMD_HEAD_SIZE];

    switch(usCmd) {
        case CMD_ID: {
            memcpy(pClient->m_strID, pData, iDataLen);
            pClient->m_nHpNo = nHpNo;

            // Mobile Controller
            if( !strcmp(pClient->m_strID, MOBILE_CONTROLL_ID) ) {
                pClient->m_nClientType = CLIENT_TYPE_MC;

                printf("Connected Mobile Controller\n");
            } else {
                pClient->m_nClientType = CLIENT_TYPE_HOST;

                m_iRefreshCH[nHpNo - 1] = 1;   // 전체 영상 전송

                printf("Connected Host : %s\n", pClient->m_strID);

                m_VPSSvr.AddClientList(pClient);
            }

            ret = true;
        }
        break;

        case CMD_ID_GUEST: {
            pClient->m_nHpNo = nHpNo;
            pClient->m_nClientType = CLIENT_TYPE_GUEST;
            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송

            m_VPSSvr.AddClientList(pClient);
        }
        break;

        case CMD_ID_MONITOR: {
            pClient->m_nHpNo = nHpNo;
            pClient->m_nClientType = CLIENT_TYPE_MONITOR;
            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송

            m_VPSSvr.AddClientList(pClient);
        }
        break;

        case CMD_PLAYER_EXIT: {
            printf("[VPS:%d] Received PLAYER EXIT\n", nHpNo);
            pClient->m_isExitCommandReceived = true;

            int nClientType = pClient->m_nClientType;

            CloseClient(pClient);   // ClientObject 객체 삭제됨

            if(nClientType == CLIENT_TYPE_GUEST) {

            }
        }
        break;

        case CMD_START: {
            short sVarHor = SwapEndianU2( *(short*)pData );
            short sVarVer = SwapEndianU2( *(short*)(pData + 2) );

            m_isOnService[nHpNo - 1] = true;

            printf("[VPS:%d] Start Command 명령 받음: LCD Width=%d, Height=%d\n", nHpNo, sVarHor, sVarVer);

            // start mirroring
            m_mirror.StartMirroring(nHpNo, DoMirrorCallback, DoMirrorStoppedCallback);
        }
        break;

        case CMD_STOP: {
            m_isOnService[nHpNo - 1] = false;

            printf("[VPS:%d] Stop Command 명령 받음\n", nHpNo);

            // test code
            m_mirror.StopMirroring(nHpNo);
        }
        break;

        case CMD_VERTICAL: {
            m_mirror.SendKeyFrame(nHpNo);

            m_mirror.SetDeviceOrientation(nHpNo, 1);
        }
        break;

        case CMD_HORIZONTAL: {
            m_mirror.SendKeyFrame(nHpNo);

            m_mirror.SetDeviceOrientation(nHpNo, 0);
        }
        break;
    }

    return ret;
}