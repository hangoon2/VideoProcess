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
    return true;
}

void DoMirrorStoppedCallback(int nHpNo, int nStopCode) {

}

NetManager::NetManager() {
    printf("Call NetManager Constructor\n");

    for(int i = 0; i < MAXCHCNT; i++) {
        m_isOnService[i] = false;
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

                printf("Connected Host : %s\n", pClient->m_strID);
            }

            ret = true;
        }
        break;

        case CMD_ID_GUEST: {

        }
        break;

        case CMD_ID_MONITOR: {

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
    }

    return ret;
}