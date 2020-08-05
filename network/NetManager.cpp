#include "NetManager.h"
#include "../common/VPSCommon.h"
#include "../mirroring/VPSJpeg.h"
#include "../recorder/VideoRecorder.h"

#include <stdio.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

using namespace cv;

static NetManager *pNetMgr = NULL;

static VideoWriter gs_writer[MAXCHCNT];
static bool is_RectReady[MAXCHCNT];
static Mat gs_lastScene[MAXCHCNT];
static int gs_nLogerKeyFrameLength[MAXCHCNT];
static int gs_nShorterKeyFrameLength[MAXCHCNT];

static VideoRecorder* gs_recorder[MAXCHCNT];

static BYTE gs_pBufSendDataToClient[MAXCHCNT + 1][SEND_BUF_SIZE] = {0,};

static void* gs_sharedMem = NULL;

void OnTimer(int id) {
    if(pNetMgr == NULL) return;

    pNetMgr->UpdateState(id);
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
#if ENABLE_SHARED_MEMORY
    gs_sharedMem = Shared_GetPointer();
#endif

    for(int i = 0; i < MAXCHCNT; i++) {
        m_isOnService[i] = false;
        
        m_nRecordStartCommandCountReceived[i] = 0;
        m_nRecordStopCommandCountReceived[i] = 0;
        m_nCaptureCommandReceivedCount[i] = 0;

        m_iRefreshCH[i] = 1;

        m_isJpgCapture[i] = false;
        m_isRunRecord[i] = false;

        is_RectReady[i] = false;
        
        gs_nLogerKeyFrameLength[i] = 0;
        gs_nShorterKeyFrameLength[i] = 0;

        gs_recorder[i] = new VideoRecorder(gs_sharedMem);
        gs_lastScene[i].create(960, 960, CV_8UC3);
    }

    memset(m_strAviSavePath, 0x00, sizeof(m_strAviSavePath));

    pNetMgr = this;

    m_timer_1.Start(TIMERID_JPGFPS_1SEC, OnTimer);
    m_timer_1.Start(TIMERID_10SEC, OnTimer);
}

NetManager::~NetManager() {
    m_timer_1.Stop();
    m_timer_10.Stop();

    for(int i = 0; i < MAXCHCNT; i++) {
        delete gs_recorder[i];
        gs_recorder[i] = NULL;
    }
}

void NetManager::OnServerModeStart() {
    GetPrivateProfileString(VPS_SZ_SECTION_CAPTURE, VPS_SZ_KEY_AVI_PATH, "", m_strAviSavePath);

    char value[16] = {0,};
    GetPrivateProfileString(VPS_SZ_SECTION_STREAM, VPS_SZ_KEY_SERVER_PORT, "10001", value);
    int port = atoi(value);

    int server = m_VPSSvr.InitSocket(this, port);
    if(server <= 0) {
        printf("Media Server Socket 생성 실패\n");
    }
}

bool NetManager::BypassPacket(void* pMirroringPacket) {
    BYTE* pPacket = (BYTE*)pMirroringPacket;

    int iDataLen = ntohl( *(uint32_t*)&pPacket[1] );
    short usCmd = ntohs( *(short*)&pPacket[5] );
    int nHpNo = *(&pPacket[7]);
    bool isKeyFrame = *&pPacket[24] == 1 ? true : false;

    if(usCmd == CMD_MIRRORING_CAPTURE_FAILED) {
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

        if(m_isJpgCapture[nHpNo - 1] && isKeyFrame) {
            m_isJpgCapture[nHpNo - 1] = false;

            BYTE* pJpgData = pPacket + 25;
            int nJpgDataSize = iDataLen - 17;

            if(usCmd == CMD_JPG_DEV_VERT_IMG_HORI) {
                VPSJpeg vpsJpeg;
                int nNewJpgDataSize = vpsJpeg.RotateLeft(pJpgData, nJpgDataSize, 90);
                if(nNewJpgDataSize > 0) {
                    nJpgDataSize = nNewJpgDataSize;
                }
            } else if(usCmd == CMD_JPG_DEV_HORI_IMG_VERT) {
                VPSJpeg vpsJpeg;
                int nNewJpgDataSize = vpsJpeg.RotateRight(pJpgData, nJpgDataSize, 90);
                if(nNewJpgDataSize > 0) {
                    nJpgDataSize = nNewJpgDataSize;
                }
            }

            return JPGCaptureAndSend(nHpNo, pJpgData, nJpgDataSize);
        } 

        DoMirrorVideoRecording(nHpNo, usCmd, isKeyFrame, pPacket, iDataLen);
    }

    return true;
}

void NetManager::OnMirrorStopped(int nHpNo, int nStopCode) {
    printf("[VPS:%d] OnMirrorStopped : %d\n", nHpNo, nStopCode);

    if( IsOnService(nHpNo) ) {
        // 아직 서비스 중인 상태에서 소켓이 닫힌 경우 DC에게 알린다.
        static BYTE pDstData[SEND_BUF_SIZE] = {0,};
        BYTE pData[2] = {0,};
        short errorCode = htons(101);
        int totLen = 0;

        memcpy(pData, &errorCode, 2);
        BYTE* pSendData = MakeSendData2(CMD_MIRRORING_CAPTURE_FAILED, nHpNo, 2, pData, pDstData, totLen);

        SendToMobileController(pSendData, totLen);

        printf("[VPS:%d] The socket is abnormally closed.\n", nHpNo);
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

void NetManager::ClientConnected(ClientObject* pClient) {
    m_VPSSvr.UpdateClientList(pClient);

    const char* strClientType = pClient->GetClientTypeString();
    printf("[VPS:%d] %s is connected.\n", pClient->m_nHpNo, strClientType);
}

void NetManager::ClientDisconnected(ClientObject* pClient) {
    const char* strClientType = pClient->GetClientTypeString();
    printf("[VPS:%d] %s is disconnected.\n", pClient->m_nHpNo, strClientType);
}

bool NetManager::SendToMobileController(BYTE* pData, int iLen, bool force) {
    ClientObject* pMobileController = m_VPSSvr.GetMobileController();
    return Send(pData, iLen, pMobileController, force);
}

bool NetManager::SendToClient(short usCmd, int nHpNo, BYTE* pData, int iLen, int iKeyFrameNo) {
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

        if( Send(pData, iLen, pClient, true) ) {
            iClientCount++;
        }
    }

    return iClientCount > 0;
}

bool NetManager::Send(BYTE* pData, int iLen, ClientObject* pClient, bool force) {
    if(pClient->m_clientSock == INVALID_SOCKET) return false;

    bool ret = false;

    pClient->Lock();

    ret = m_VPSSvr.OnSend(pClient->m_clientSock, pData, iLen, force);
    if(ret) {
        // client 데이터 전송량 업데이트
    }

    pClient->Unlock();

    return ret;
}

bool NetManager::JPGCaptureAndSend(int nHpNo, BYTE* pJpgData, int iJpgDataLen) {
    char filePath[512] = {0,};
    char fileName[DEFAULT_STRING_SIZE] = {0,};

    SYSTEM_TIME stTime;
    GetLocalTime(stTime);

    sprintf(fileName, "%02d_%04d%02d%02d_%02d%02d%02d%03d.jpg",
            nHpNo, 
            stTime.year, stTime.month, stTime.day,
            stTime.hour, stTime.minute, stTime.second, stTime.millisecond);

    sprintf(filePath, "%s/%d/%s", m_strAviSavePath, nHpNo, fileName);

    VPSJpeg jpegEncoder;
    if( jpegEncoder.SaveJpeg(filePath, pJpgData, iJpgDataLen, 90) ) {
        // 파일 존재 유무 체크
        bool doesFileExist = DoesFileExist(filePath);
        if(!doesFileExist) {
            sprintf(fileName, "%s", VPS_SZ_JPG_CREATION_FAIL);
        }

        memset(gs_pBufSendDataToClient[MAXCHCNT], 0x00, SEND_BUF_SIZE);

        int totLen = 0;
        BYTE* pSendData = MakeSendData2(CMD_CAPTURE_COMPLETED, nHpNo, (int)strlen(fileName), (BYTE*)fileName, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
        if( SendToMobileController(pSendData, totLen) ) {
            printf("[VPS:%d] [Capture] 단말기 화면 캡쳐 응답보냄: 성공(%s)\n", nHpNo, fileName);
            return true;
        } else {
            printf("[VPS:%d] [Capture] 단말기 화면 캡쳐 응답보냄: 실패(%s)\n", nHpNo, fileName);
        }
    }

    return false;
}

void NetManager::Record(int nHpNo, bool start) {
    if(start) {
        char filePath[512] = {0,};
        char fileName[DEFAULT_STRING_SIZE] = {0,};

        SYSTEM_TIME stTime;
        GetLocalTime(stTime);

        sprintf(fileName, "%02d_%04d%02d%02d_%02d%02d%02d%03d.mp4",
                nHpNo, 
                stTime.year, stTime.month, stTime.day,
                stTime.hour, stTime.minute, stTime.second, stTime.millisecond);

        sprintf(filePath, "%s/%d/%s", m_strAviSavePath, nHpNo, fileName);

        is_RectReady[nHpNo - 1] = false;

        // int codec = VideoWriter::fourcc('f', 'l', 'v', '1');
        // double fps = 20.0;
        // Size sizeFrame(960, 960);
        // gs_writer[nHpNo - 1].open(filePath, codec, fps, sizeFrame, true);

        gs_recorder[nHpNo - 1]->StartRecord(nHpNo, filePath);

        m_isRunRecord[nHpNo - 1] = start;

        m_mirror.SendKeyFrame(nHpNo);

        m_nRecordStartCommandCountReceived[nHpNo - 1]++;
    } else {
        m_isRunRecord[nHpNo - 1] = start;

        // gs_writer[nHpNo - 1].release();
        
        gs_recorder[nHpNo - 1]->StopRecord();

        m_nRecordStopCommandCountReceived[nHpNo - 1]++;
    }
}

bool NetManager::DoMirrorVideoRecording(int nHpNo, short usCmd, bool isKeyFrame, BYTE* pPacket, int iDataLen) {
    BYTE* pJpgSrc = pPacket + 25;
    int nJpgSrcLen = iDataLen - 17;
    
    short nLeft = ntohs( *(short*)&pPacket[16] );
    short nTop = ntohs( *(short*)&pPacket[18] );
    short nRight = ntohs( *(short*)&pPacket[20] );
    short nBottom = ntohs( *(short*)&pPacket[22] );

    if(isKeyFrame) {
        if(m_isRunRecord[nHpNo - 1] && !is_RectReady[nHpNo - 1]) {
            is_RectReady[nHpNo - 1] = true;
        }

        gs_nLogerKeyFrameLength[nHpNo - 1] = nRight > nBottom ? nRight : nBottom;
        gs_nShorterKeyFrameLength[nHpNo - 1] = nRight < nBottom ? nRight : nBottom;

        memset( gs_lastScene[nHpNo - 1].data, 0x00, gs_lastScene[nHpNo - 1].total() * gs_lastScene[nHpNo - 1].elemSize() );
    }

    Rect rect;

    if(usCmd == CMD_JPG_DEV_VERT_IMG_HORI) {
        VPSJpeg vpsJpeg;
        int nNewJpgDataSize = vpsJpeg.RotateLeft(pJpgSrc, nJpgSrcLen, 90);
        if(nNewJpgDataSize > 0) {
            nJpgSrcLen = nNewJpgDataSize;
            usCmd = CMD_JPG_DEV_HORI_IMG_HORI;

            short nTempLeft = nLeft;
            nLeft = nTop;
            nTop = gs_nShorterKeyFrameLength[nHpNo - 1] - nRight;
            nRight = nBottom;
            nBottom = gs_nShorterKeyFrameLength[nHpNo - 1] - nTempLeft;
        }
    } else if(usCmd == CMD_JPG_DEV_HORI_IMG_VERT) {
        VPSJpeg vpsJpeg;
        int nNewJpgDataSize = vpsJpeg.RotateRight(pJpgSrc, nJpgSrcLen, 90);
        if(nNewJpgDataSize > 0) {
            nJpgSrcLen = nNewJpgDataSize;
            usCmd = CMD_JPG_DEV_VERT_IMG_VERT;

            short nTempLeft = nLeft;
            short nTempTop = nTop;
            short nTempRight = nRight;
            nLeft = gs_nShorterKeyFrameLength[nHpNo - 1] - nBottom;
            nTop = nTempLeft;
            nRight = gs_nShorterKeyFrameLength[nHpNo - 1] - nTempTop;
            nBottom = nTempRight;
        }
    }

    Mat rawData = Mat(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc).clone();
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);

    int startPt = (gs_nLogerKeyFrameLength[nHpNo - 1] - gs_nShorterKeyFrameLength[nHpNo - 1]) / 2;

    if(isKeyFrame) {
        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT) {
            rect.x = startPt;
            rect.y = 0;
            rect.width = nRight - nLeft;
            rect.height = nBottom - nTop;
        } else if(usCmd == CMD_JPG_DEV_HORI_IMG_HORI) {
            rect.x = 0;
            rect.y = startPt;
            rect.width = nRight - nLeft;
            rect.height = nBottom - nTop;
        }
    } else {
        if(usCmd == CMD_JPG_DEV_VERT_IMG_VERT) {
            rect.x = startPt + nLeft;
            rect.y = nTop;
            rect.width = nRight - nLeft;
            rect.height = nBottom - nTop;
        } else if(usCmd == CMD_JPG_DEV_HORI_IMG_HORI) {
            rect.x = nLeft;
            rect.y = startPt + nTop;
            rect.width = nRight - nLeft;
            rect.height = nBottom - nTop;
        }
    }

    Mat imageROI(gs_lastScene[nHpNo - 1], rect);
    rawImage.copyTo(imageROI);

    if(!isKeyFrame && !is_RectReady[nHpNo - 1]) {
        return false;
    }

    gs_recorder[nHpNo - 1]->EnQueue(gs_lastScene[nHpNo - 1].data);

    return true;
}

bool NetManager::CloseClient(ClientObject* pClient) {
    return m_VPSSvr.OnClose(pClient->m_clientSock);
}

bool NetManager::WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len) {
    int iDataLen = SwapEndianU4( *(int*)&pRcvData[1] );
    short usCmd = SwapEndianU2( *(short*)&pRcvData[5] );
    int nHpNo = (int)pRcvData[7];

//    printf("Received Data : %d, %d, %d\n", nHpNo, usCmd, iDataLen);

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

//    uint16_t usChkRcv = *(uint16_t*)&pRcvData[CMD_HEAD_SIZE + iDataLen];
//    uint16_t usChk = calChecksum( (ONYPACKET_UINT16*)&pRcvData[1], 7 + iDataLen);
//    if(usChkRcv != usChk) {
//        printf("***** Wrong Check Sum  => Rcv: %X, Calc: %X\n", usChkRcv, usChk);
//        return false;
//    }

    bool ret = false;
    char* pData = &pRcvData[CMD_HEAD_SIZE];

    switch(usCmd) {
        case CMD_LOGCAT:
        case CMD_ACK:
        case CMD_TEST_RESULT:
        case CMD_TEST_START_EVENT_INDEX:
        case CMD_TEST_START_EVENT_PATH:
        case CMD_TEST_START_SCRIPT_RESULT:
        case CMD_RESOURCE_USAGE_NETWORK:
        case CMD_RESOURCE_USAGE_CPU:
        case CMD_RESOURCE_USAGE_MEMORY: {
            return SendToClient(usCmd, nHpNo, (BYTE*)pRcvData, len, 0);
        }
        break;

        case CMD_ID: {
            memcpy(pClient->m_strID, pData, iDataLen);
            pClient->m_nHpNo = nHpNo;

            // Mobile Controller
            if( !strcmp(pClient->m_strID, MOBILE_CONTROLL_ID) ) {
                pClient->m_nClientType = CLIENT_TYPE_MC;
            } else {
                pClient->m_nClientType = CLIENT_TYPE_HOST;

                m_iRefreshCH[nHpNo - 1] = 1;   // 전체 영상 전송
            }

            ClientConnected(pClient);
        }
        break;

        case CMD_ID_GUEST: {
            memcpy(pClient->m_strID, pData, iDataLen);
            pClient->m_nHpNo = nHpNo;
            pClient->m_nClientType = CLIENT_TYPE_GUEST;
            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송

            // Guest 연결될때 해당 채널의 Host가 연결되어 있지 않으면 연결을 허락하지 않음
            ClientObject* pHost = m_VPSSvr.FindHost(nHpNo);
            if(pHost == NULL) {
                int totLen = 0;
                BYTE* pSendData = MakeSendData2(CMD_DISCONNECT_GUEST, nHpNo, iDataLen, (BYTE*)pData, gs_pBufSendDataToClient[nHpNo - 1], totLen);
                Send(pSendData, totLen, pClient, false);

                return true;
            } else {
                // 디바이스와의 영상은 이미 연결된 상태이므로 Keyframe을 요청한다.
                m_mirror.SendKeyFrame(nHpNo);

                int totLen = 0;
                BYTE* pSendData = MakeSendData2(CMD_GUEST_UPDATED, nHpNo, 0, NULL, gs_pBufSendDataToClient[nHpNo - 1], totLen);
                Send(pSendData, totLen, pHost, false);
            }

            ClientConnected(pClient);
        }
        break;

        case CMD_ID_MONITOR: {
            memcpy(pClient->m_strID, pData, iDataLen);
            pClient->m_nHpNo = nHpNo;
            pClient->m_nClientType = CLIENT_TYPE_MONITOR;
            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송

            ClientConnected(pClient);
        }
        break;

        case CMD_PLAYER_EXIT: {
            printf("[VPS:%d] Received PLAYER EXIT\n", nHpNo);
            pClient->m_isExitCommandReceived = true;

            int nClientType = pClient->m_nClientType;

            CloseClient(pClient);   // ClientObject 객체 삭제됨

            if(nClientType == CLIENT_TYPE_GUEST) {
                ClientObject* pHost = m_VPSSvr.FindHost(nHpNo);
                if(pHost != NULL) {
                    int totLen = 0;
                    BYTE* pSendData = MakeSendData2(CMD_GUEST_UPDATED, nHpNo, 0, NULL, gs_pBufSendDataToClient[nHpNo - 1], totLen);
                    Send(pSendData, totLen, pHost, false);
                }
            }
        }
        break;

        case CMD_DISCONNECT_GUEST: {
            char id[DEFAULT_STRING_SIZE] = {0,};
            memcpy(id, pData, iDataLen);

            ClientObject* pGuest = m_VPSSvr.FindGuest(nHpNo, id);
            if(pGuest != NULL) {
                if( !Send((BYTE*)pRcvData, len, pGuest, false) ) {
                    // Guest에 DISCONNECT 전송 실패 시 강제 종료 시킴
                    m_VPSSvr.OnClose(pGuest->m_clientSock);
                }
            }
        }
        break;

        case CMD_UPDATE_SERVICE_TIME: {
            ClientObject** pClientList = m_VPSSvr.GetClientList(nHpNo);
            for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
                ClientObject* pGuest = pClientList[i];
                if(pGuest == NULL) continue;

                if(pGuest->m_nClientType == CLIENT_TYPE_GUEST) {
                    Send((BYTE*)pRcvData, len, pGuest, false);
                }
            }
        }
        break;

        case CMD_DEVICE_DISCONNECTED: {
            ClientObject** pClientList = m_VPSSvr.GetClientList(nHpNo);
            for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
                ClientObject* pClientObj = pClientList[i];
                if(pClientObj == NULL) continue;

                if(pClientObj->m_nClientType == CLIENT_TYPE_HOST
                    && pClientObj->m_nClientType == CLIENT_TYPE_MONITOR) {
                    Send((BYTE*)pRcvData, len, pClientObj, false);
                }
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

            // 기존 VPS에서는 CMD_PLAYER_QUALITY 명령을 보낸다.
            // 안드로이드 영상 프로세스(nxptc)는 기본 70으로 되어 있어서 보낼 필요 없음
            // ios 영상 프로세스(onycap)는 확인 필요, 우선 처리 보류
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

            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송
            m_mirror.SetDeviceOrientation(nHpNo, 1);
        }
        break;

        case CMD_HORIZONTAL: {
            m_mirror.SendKeyFrame(nHpNo);

            m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송
            m_mirror.SetDeviceOrientation(nHpNo, 0);
        }
        break;

        case CMD_RECORD: {
            bool start = pData[0] == 1 ? true : false;

            printf("[VPS:%d] [Record] 단말기 녹화 %s 명령 받음\n", nHpNo, start ? "시작" : "정지");

            // 녹화 명령 처리
            Record(nHpNo, start);
        }
        break;

        case CMD_JPG_CAPTURE: {
            m_nCaptureCommandReceivedCount[nHpNo - 1]++;

            m_isJpgCapture[nHpNo - 1] = true;

            printf("[VPS:%d] [Capture] 단말기 화면 캡쳐 명령 받음\n", nHpNo);

            m_mirror.SendKeyFrame(nHpNo);
        }
        break;

        case CMD_PLAYER_QUALITY: {
            m_mirror.SendControlPacket(nHpNo, (BYTE*)pRcvData, len);
        }
        break;

        case CMD_KEY_FRAME: {
            m_mirror.SendKeyFrame(nHpNo);
        }
        break;

        //////////// 사용 하지 않는 패킷(?) ////////////////////
        case CMD_PLAYER_FPS: {
            // 캡쳐 보드에서 사용(?)
        }
        break; 

        case CMD_WAKEUP: {

        }
        break;

        case CMD_CHANGE_RESOLUTION: {

        }
        break;

        case CMD_CHANGE_RESOLUTION_RATIO: {
            // 녹화 중이면 해상도를 변경하지 않음
            
        }
        break;

        case CMD_REQUEST_MAX_RESOLUTION: {

        }
        break;
        /////////////////////////////////////////////////////

        case CMD_MONITOR_VD_HEARTBEAT: {
            // 별도의 처리는 하지 않는다.
        }
        break;

        default: {
            return SendToMobileController((BYTE*)pRcvData, len);
        }
        break;
    }

    return ret;
}

void NetManager::UpdateState(int id) {
    switch(id) {
        case TIMERID_JPGFPS_1SEC: {
             
        }
        break;

        case TIMERID_10SEC: {

        }
        break;
    }
}