#include "NetManager.h"
#include "../common/VPSCommon.h"

#include <stdio.h>

static NetManager *pNetMgr = NULL;

bool OnReadEx(ClientObject* pObject, char* pData, int len) {
    if(pNetMgr != NULL)
        return pNetMgr->OnReadEx(pObject, pData, len);

    return false;
}

NetManager::NetManager() {
    printf("Call NetManager Constructor\n");

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

bool NetManager::OnReadEx(ClientObject* pObject, char* pData, int len) {
    printf("NetManager::OnReadEx Socket(%d) ----> %d\n", pObject->m_clientSock, len);
    if(pData[0] == CMD_START_CODE && pObject->m_rcvCommandBuffer[0] != CMD_START_CODE) {
        memcpy(pObject->m_rcvCommandBuffer, pData, len);
        pObject->m_rcvPos = len;
    } else {
        // 처음이 시작코드가 아니면 버린다. 
        // 항상 시작 코드부터 패킷을 만든다.
        if(pObject->m_rcvCommandBuffer[0] != CMD_START_CODE)
            return false;

        memcpy(&pObject->m_rcvCommandBuffer[pObject->m_rcvPos], pData, len);
        pObject->m_rcvPos += len;
    }

    while(1) {
        if(pObject->m_rcvCommandBuffer[0] != CMD_START_CODE) break;

        bool bPacketOK = false;
        int iDataLen = (*(int*)&pObject->m_rcvCommandBuffer[1]);
        int totDataLen = CMD_HEAD_SIZE + iDataLen + CMD_TAIL_SIZE;
        if(pObject->m_rcvPos >= totDataLen) {
            if(pObject->m_rcvCommandBuffer[totDataLen - 1]) {
                bPacketOK = true;
            }
        }

        // 패킷이 완성될때까지 반복
        if(bPacketOK == false) return false;

        WebCommandDataParsing2(pObject, pObject->m_rcvCommandBuffer, totDataLen);

        pObject->m_rcvCommandBuffer[0] = 0; // 처리 완료 플래그

        // 처리 후, 남은 패킷을 맨앞으로 당겨준다.
    }

    return true;
}

bool NetManager::WebCommandDataParsing2(ClientObject* pObject, char* pData, int len) {
    return false;
}