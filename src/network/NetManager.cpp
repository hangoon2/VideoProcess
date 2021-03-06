#include "NetManager.h"
#include "../common/VPSCommon.h"
#include "../mirroring/VPSJpeg.h"
#include "../recorder/VideoRecorder.h"

#include <stdio.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

using namespace cv;

static const BYTE VIDEO_FPS_STATUS_NO_CHANGE = 0;   // FPS 상태가 기존과 동일
static const BYTE VIDEO_FPS_STATUS_SUCCESS = 1;     // FPS 가 BASE FPS 보다 크거나 같은 경우
static const BYTE VIDEO_FPS_STATUS_FAILURE = 2;     // FPS 가 BASE FPS 보다 작은 경우

static NetManager *pNetMgr = NULL;

static bool is_RectReady[MAXCHCNT];
static Mat gs_lastScene[MAXCHCNT];
static int gs_nLogerKeyFrameLength[MAXCHCNT];
static int gs_nShorterKeyFrameLength[MAXCHCNT];

static VideoRecorder* gs_recorder[MAXCHCNT];
static VPSJpeg gs_mirJpeg[MAXCHCNT];

static char gs_recordFileName[MAXCHCNT][DEFAULT_STRING_SIZE];

static BYTE gs_pBufSendDataToClient[MAXCHCNT + 1][SEND_BUF_SIZE];

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

void DoRecordStopCallback(int nHpNo) {
    if(pNetMgr != NULL) {
        pNetMgr->RecordStop(nHpNo);
    }
}

// bool DoMirrorRecordCallback(void* pMirroringPacket) {
//     // if(pNetMgr != NULL) {
//     //     BYTE* pPacket = (BYTE*)pMirroringPacket;

//     //     int iDataLen = ntohl( *(uint32_t*)&pPacket[1] );
//     //     short usCmd = ntohs( *(short*)&pPacket[5] );
//     //     int nHpNo = *(&pPacket[7]);
//     //     bool isKeyFrame = *&pPacket[24] == 1 ? true : false;

//     //     pNetMgr->DoMirrorVideoRecording(nHpNo, usCmd, isKeyFrame, pPacket, iDataLen);
//     // }

//     return false;
// }

void DoAddLogCallback(int nHpNo, const char* log, vps_log_target_t nTarget) {
    if(pNetMgr != NULL) {
        pNetMgr->AddLog(nHpNo, log, nTarget);
    }
}

NetManager::NetManager(PVPS_CALLBACK_ROUTINE fnCallback) {
    m_fnCallback = fnCallback;

    char value[16] = {0,};
    GetPrivateProfileString(VPS_SZ_SECTION_STREAM, VPS_SZ_KEY_SERVER_PORT, "10001", value);
    m_serverPort = atoi(value);

#if ENABLE_SHARED_MEMORY
    gs_sharedMem = Shared_GetPointer();
#endif

    for(int i = 0; i < MAXCHCNT; i++) {
        m_isOnService[i] = false;
        
        m_isReceivedRecordStartCommand[i] = false;
        m_nRecordStartCommandCountReceived[i] = 0;
        m_nRecordStopCommandCountReceived[i] = 0;
        m_nRecordCompletionCountSent[i] = 0;
        
        m_nCaptureCommandReceivedCount[i] = 0;
        m_nCaptureCompletionCountSent[i] = 0;

        m_iMirrorVideoInputCount[i] = 0;

        m_iCaptureCount[i] = 0;
        m_iCaptureCountOld[i] = 0;
        m_iCompressedCount[i] = 0;
        m_iCompressedCountOld[i] = 0;
        m_iSendCount[i] = 0;
        m_iSendCountOld[i] = 0;

        m_iRefreshCH[i] = 1;

        m_isToSkipFailureReport[i] = false;

        m_isJpgCapture[i] = false;
        m_nJpgCaptureStartTime[i] = 0;

        m_isRunRecord[i] = false;
        m_nRecStartTime[i] = 0;

        is_RectReady[i] = false;
        
        gs_nLogerKeyFrameLength[i] = 0;
        gs_nShorterKeyFrameLength[i] = 0;

        gs_recorder[i] = new VideoRecorder(gs_sharedMem, i + 1, m_serverPort);
        gs_lastScene[i].create(MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, CV_8UC3);
    }

    memset( m_strAviSavePath, 0x00, sizeof(m_strAviSavePath) );

    memset( m_byOldVideoFpsStatus, 0x00, sizeof(m_byOldVideoFpsStatus) );

    memset( gs_recordFileName, 0x00, MAXCHCNT * DEFAULT_STRING_SIZE );
    memset( gs_pBufSendDataToClient, 0x00, (MAXCHCNT + 1) * SEND_BUF_SIZE );

    pNetMgr = this;

    m_timer_1.Start(TIMERID_JPGFPS_1SEC, OnTimer);
    m_timer_10.Start(TIMERID_10SEC, OnTimer);
    m_timer_15.Start(TIMERID_15SEC, OnTimer);
    m_timer_20.Start(TIMERID_20SEC, OnTimer);

    m_mirror.Initialize(NULL, DoAddLogCallback);
}

NetManager::~NetManager() {
    AddLog(0, "VPS 종료", LOG_TO_FILE);
}

void NetManager::OnServerModeStart() {
    GetPrivateProfileString(VPS_SZ_SECTION_CAPTURE, VPS_SZ_KEY_AVI_PATH, "", m_strAviSavePath);

    int server = m_VPSSvr.InitSocket(this, m_serverPort);
    if(server < 0) {
        AddLog(0, "Media Server Socket 생성 실패", LOG_TO_FILE);
    }
}

void NetManager::OnDestroy() {
    for(int i = 0; i < MAXCHCNT; i++) {
        if(m_isOnService[i]) {
            m_isOnService[i] = false;

            CloseClientManualEx(i + 1);

            usleep(1);
        }

        delete gs_recorder[i];
        gs_recorder[i] = NULL;

        usleep(1);
    }

    m_timer_1.Stop();
    m_timer_10.Stop();
    m_timer_15.Stop();
    m_timer_20.Stop();

    m_VPSSvr.DestSocket();
}

bool NetManager::BypassPacket(void* pMirroringPacket) {
    BYTE* pPacket = (BYTE*)pMirroringPacket;

    int iDataLen = ntohl( *(uint32_t*)&pPacket[1] );
    short usCmd = ntohs( *(short*)&pPacket[5] );
    int nHpNo = *(&pPacket[7]);
    bool isKeyFrame = *&pPacket[24] == 1 ? true : false;

    // capture count 업데이트
    m_iCaptureCount[nHpNo - 1]++;

    // 장애처리를 위한 mirroring count 업데이트
    m_iMirrorVideoInputCount[nHpNo - 1]++;

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
            m_iSendCount[nHpNo - 1]++;
        } else {
            // 전송 실패 시 키프레임을 요청한다.
            m_mirror.SendKeyFrame(nHpNo); 
        }

        DoMirrorVideoRecording(nHpNo, usCmd, isKeyFrame, pPacket, iDataLen);
    }

    return true;
}

void NetManager::OnMirrorStopped(int nHpNo, int nStopCode) {
    if( IsOnService(nHpNo) ) {
        m_isOnService[nHpNo - 1] = false;

        // 아직 서비스 중인 상태에서 소켓이 닫힌 경우 DC에게 알린다.
        static BYTE pDstData[SEND_BUF_SIZE] = {0,};
        BYTE pData[2] = {0,};
        short errorCode = htons(101);
        int totLen = 0;

        memcpy(pData, &errorCode, 2);
        BYTE* pSendData = MakeSendData2(CMD_MIRRORING_CAPTURE_FAILED, nHpNo, 2, pData, pDstData, totLen);

        SendToMobileController(pSendData, totLen);

        AddLog(nHpNo, "The socket is abnormally closed.", LOG_TO_BOTH);
    } else {
        char log[128] = {0,};
        sprintf(log, "OnMirrorStopped : %d", nStopCode);
        AddLog(nHpNo, log, LOG_TO_FILE);
    }
}

bool NetManager::OnReadEx(ClientObject* pClient, char* pRcvData, int len) {
    if(pClient == NULL) return false;

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

        Socket sock = pClient->m_clientSock;

        WebCommandDataParsing2(pClient, pClient->m_rcvCommandBuffer, totDataLen);

        if( m_VPSSvr.Find(sock) == NULL ) {
            // WebCommandDataParsing2 처리에서 close socket이 처리된 경우 해당 객체가 삭제된다.
            return false;
        }
        
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
    int nHpNo = pClient->m_nHpNo;
    int nClientType = pClient->m_nClientType;

    m_VPSSvr.UpdateClientList(pClient);

    m_isToSkipFailureReport[nHpNo - 1] = true;

    char log[128] = {0,};
    sprintf(log, "%s is connected.", pClient->GetClientTypeString());
    AddLog(nHpNo, log, LOG_TO_BOTH);    

    if( (nClientType == CLIENT_TYPE_HOST || nClientType == CLIENT_TYPE_MONITOR)
            && m_fnCallback != NULL ) {
        CALLBACK cb;

        cb.nHpNo = nHpNo;
        cb.type = CALLBACK_TYPE_HOST_CONNECT;
        sprintf(cb.data, "%s/%s", pClient->m_strID, pClient->m_strIPAddr);

        m_fnCallback(&cb);
    }
}

void NetManager::ClientDisconnected(ClientObject* pClient) {
    int nHpNo = pClient->m_nHpNo;
    int nClientType = pClient->m_nClientType;

    m_isToSkipFailureReport[nHpNo - 1] = false;

    m_lockCurrenScene[nHpNo - 1].Lock();

    m_lockCurrenScene[nHpNo - 1].Unlock();

    char log[128] = {0,};
    sprintf(log, "%s is disconnected.", pClient->GetClientTypeString());
    AddLog(nHpNo, log, LOG_TO_BOTH);  

    if( (nClientType == CLIENT_TYPE_HOST || nClientType == CLIENT_TYPE_MONITOR)
            && m_fnCallback != NULL ) {
        CALLBACK cb;

        cb.nHpNo = nHpNo;
        cb.type = CALLBACK_TYPE_HOST_DISCONNECT;

        m_fnCallback(&cb);
    }
}

void NetManager::DeviceRotate(int nHpNo, bool portrait) {
    m_mirror.SendKeyFrame(nHpNo);

    m_iRefreshCH[nHpNo - 1] = 1;    // 전체 영상 전송
    m_mirror.SetDeviceOrientation(nHpNo, portrait ? 1 : 0);

    if(portrait) {
        AddLog(nHpNo, "영상 세로 모드 출력", LOG_TO_BOTH);  
    } else {
        AddLog(nHpNo, "영상 가로 모드 출력", LOG_TO_BOTH);  
    }
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

        if( Send(pData, iLen, pClient, false) ) {
            iClientCount++;
        }
    }

    return iClientCount > 0;
}

bool NetManager::Send(BYTE* pData, int iLen, ClientObject* pClient, bool force) {
    if(pClient == NULL || pClient->m_clientSock == INVALID_SOCKET) return false;

    bool ret = false;

    pClient->Lock();

    ret = m_VPSSvr.OnSend(pClient, pData, iLen, force);
    if(ret) {
        // client 데이터 전송량 업데이트
        pClient->m_sendBytes += iLen;
    }

    pClient->Unlock();

    return ret;
}

bool NetManager::JPGCaptureAndSend(int nHpNo, BYTE* pJpgData, int iJpgDataLen, int width, int height) {
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
    if( jpegEncoder.EncodeJpeg(filePath, pJpgData, iJpgDataLen, width, height, 90) ) {
        // 파일 존재 유무 체크
        bool doesFileExist = DoesFileExist(filePath);
        if(!doesFileExist) {
            sprintf(fileName, "%s", VPS_SZ_JPG_CREATION_FAIL);
        }

        memset(gs_pBufSendDataToClient[MAXCHCNT], 0x00, SEND_BUF_SIZE);

        int totLen = 0;
        BYTE* pSendData = MakeSendData2(CMD_CAPTURE_COMPLETED, nHpNo, (int)strlen(fileName), (BYTE*)fileName, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
        if( SendToMobileController(pSendData, totLen) ) {
            m_nCaptureCompletionCountSent[nHpNo - 1]++;

            char log[128] = {0,};
            sprintf(log, "[Capture] 단말기 화면 캡쳐 응답보냄: 성공(%s)", fileName);
            AddLog(nHpNo, log, LOG_TO_BOTH);    

            m_isJpgCapture[nHpNo - 1] = false;
            m_nJpgCaptureStartTime[nHpNo - 1] = 0;
            return true;
        } else {
            char log[128] = {0,};
            sprintf(log, "[Capture] 단말기 화면 캡쳐 응답보냄: 실패(%s)", fileName);
            AddLog(nHpNo, log, LOG_TO_BOTH);    
        }
    }

    return false;
}

bool NetManager::JPGCaptureFailSend(int nHpNo) {
    char fileName[DEFAULT_STRING_SIZE] = {0,};

    sprintf(fileName, "%s", VPS_SZ_JPG_CREATION_FAIL);

    memset(gs_pBufSendDataToClient[MAXCHCNT], 0x00, SEND_BUF_SIZE);

    int totLen = 0;
    BYTE* pSendData = MakeSendData2(CMD_CAPTURE_COMPLETED, nHpNo, (int)strlen(fileName), (BYTE*)fileName, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
    if( SendToMobileController(pSendData, totLen) ) {
        m_nCaptureCompletionCountSent[nHpNo - 1]++;

        char log[128] = {0,};
        sprintf(log, "[Capture] 단말기 화면 캡쳐 응답보냄: 실패(%s)", fileName);
        AddLog(nHpNo, log, LOG_TO_BOTH);    

        return true;
    }

    return false;
}

bool NetManager::RecordStopAndSend(int nHpNo) {
    char filePath[512] = {0,};

    sprintf(filePath, "%s/%d/%s", m_strAviSavePath, nHpNo, gs_recordFileName[nHpNo - 1]);

    // 파일 존재 유무 체크
    bool doesFileExist = DoesFileExist(filePath);
    if(!doesFileExist) {
        sprintf(gs_recordFileName[nHpNo - 1], "%s", VPS_SZ_JPG_CREATION_FAIL);
    }

    memset(gs_pBufSendDataToClient[MAXCHCNT], 0x00, SEND_BUF_SIZE);

    int totLen = 0;
    BYTE* pSendData = MakeSendData2(CMD_CAPTURE_COMPLETED, nHpNo, (int)strlen(gs_recordFileName[nHpNo - 1]), (BYTE*)gs_recordFileName[nHpNo - 1], (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
    if( SendToMobileController(pSendData, totLen) ) {
        m_nRecordCompletionCountSent[nHpNo - 1]++;

        char log[128] = {0,};
        sprintf(log, "[Capture] 단말기 녹화 정지 응답보냄: 성공(%s)", gs_recordFileName[nHpNo - 1]);
        AddLog(nHpNo, log, LOG_TO_BOTH);    

        return true;
    } else {
        char log[128] = {0,};
        sprintf(log, "[Capture] 단말기 녹화 정지 응답보냄: 실패(%s)", gs_recordFileName[nHpNo - 1]);
        AddLog(nHpNo, log, LOG_TO_BOTH);   
    }

    return false;
}

bool NetManager::RecordStopSend(int nHpNo) {
    ClientObject* pHost = m_VPSSvr.FindHost(nHpNo);
    if(pHost != NULL) {
        int totLen = 0;
        BYTE* pSendData = MakeSendData2(CMD_STOP_RECORDING, nHpNo, 0, NULL, (BYTE*)gs_pBufSendDataToClient[nHpNo - 1], totLen);
        if( Send(pSendData, totLen, pHost, false) ) {
            return true;
        }
    }

    return false;
}

bool NetManager::SendLastRecordStopResponse(int nHpNo) {
    return RecordStopAndSend(nHpNo);
}

void NetManager::Record(int nHpNo, bool start) {
    char log[128] = {0,};
    sprintf(log, "[Record] 단말기 녹화 %s 명령 받음", start ? "시작" : "정지");
    AddLog(nHpNo, log, LOG_TO_BOTH);  

    if(start) {
        char filePath[512] = {0,};
    
        SYSTEM_TIME stTime;
        GetLocalTime(stTime);

        sprintf(gs_recordFileName[nHpNo - 1], "%02d_%04d%02d%02d_%02d%02d%02d%03d.mp4",
                nHpNo, 
                stTime.year, stTime.month, stTime.day,
                stTime.hour, stTime.minute, stTime.second, stTime.millisecond);

        sprintf(filePath, "%s/%d/%s", m_strAviSavePath, nHpNo, gs_recordFileName[nHpNo - 1]);

        gs_recorder[nHpNo - 1]->StartRecord(filePath, DoRecordStopCallback);

        m_isRunRecord[nHpNo - 1] = true;
        m_nRecStartTime[nHpNo - 1] = GetTickCount();

        m_mirror.SendKeyFrame(nHpNo);        
    } else {
        if(!m_isRunRecord[nHpNo - 1] && m_isReceivedRecordStartCommand[nHpNo - 1]) {
            // 이미 레코드 종료된 상태이므로 마지막 file name을 DC로 보내준다.
            SendLastRecordStopResponse(nHpNo);
        }

        m_isRunRecord[nHpNo - 1] = false;
        m_nRecStartTime[nHpNo - 1] = 0;
        is_RectReady[nHpNo - 1] = false;

        gs_recorder[nHpNo - 1]->StopRecord();
    }

    if(m_fnCallback != NULL) {
        CALLBACK cb;

        cb.nHpNo = nHpNo;
        cb.type = start ? CALLBACK_TYPE_RECORD_START : CALLBACK_TYPE_RECORD_STOP;

        m_fnCallback(&cb);
    }
}

void NetManager::RecordStop(int nHpNo) {
    if(m_isReceivedRecordStartCommand[nHpNo - 1]) {
        // VPS 자체적으로 종료한 경우(33분) DC로 알리지 않는다.
        // 추후 Recording Stop을 DC로 부터 받으면 그때 알려준다.
        // file name을 저장해 둔다. : gs_recordFileName[nHpNo - 1]
    } else {
        RecordStopAndSend(nHpNo);
    }
}

bool NetManager::DoMirrorVideoRecording(int nHpNo, short usCmd, bool isKeyFrame, BYTE* pPacket, int iDataLen) {
    BYTE* pJpgSrc = pPacket + 25;
    int nJpgSrcLen = iDataLen - 17;

    short nLeft = ntohs( *(short*)&pPacket[16] );
    short nTop = ntohs( *(short*)&pPacket[18] );
    short nRight = ntohs( *(short*)&pPacket[20] );
    short nBottom = ntohs( *(short*)&pPacket[22] );

    int quality = 90;

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
        int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateLeft( pJpgSrc, nJpgSrcLen, quality, (nRight - nLeft), (nBottom - nTop) );
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
        int nNewJpgDataSize = gs_mirJpeg[nHpNo - 1].RotateRight( pJpgSrc, nJpgSrcLen, quality, (nRight - nLeft), (nBottom - nTop) );
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

    if(m_isJpgCapture[nHpNo - 1] && isKeyFrame) {
        JPGCaptureAndSend( nHpNo, pJpgSrc, nJpgSrcLen, (nRight - nLeft), (nBottom - nTop) );
    } 

    BYTE* pDecodeData = gs_mirJpeg[nHpNo - 1].Decode_Jpeg(pJpgSrc, nJpgSrcLen);
    Mat rawImage((nBottom - nTop), (nRight - nLeft), CV_8UC3, pDecodeData);

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

    if(rect.x + rect.width > MAX_SIZE_SCREEN || rect.y + rect.height > MAX_SIZE_SCREEN) {
        return false;
    }

    m_lockCurrenScene[nHpNo - 1].Lock();

    Mat imageROI(gs_lastScene[nHpNo - 1], rect);
    rawImage.copyTo(imageROI);

    m_lockCurrenScene[nHpNo - 1].Unlock();

    if(is_RectReady[nHpNo - 1]) {
        if( gs_recorder[nHpNo - 1]->EnQueue(gs_lastScene[nHpNo - 1].data) ) {
            // compress count 업데이트
            m_iCompressedCount[nHpNo - 1]++;
        }
    }

    return true;
}

bool NetManager::HeartbeatSendToDC() {
    if(m_VPSSvr.GetMobileController() == NULL) return false;

    int totLen = 0;
    BYTE* pSendData = MakeSendData2(CMD_MONITOR_VPS_HEARTBEAT, 0, 0, NULL, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
    return SendToMobileController(pSendData, totLen);
}

bool NetManager::VideoFpsCheckAndSend(void* pData) {
    if(m_VPSSvr.GetMobileController() == NULL) return false;

    const int BASE_MIRRORING_VIDEO_COUNT_PER_10SEC = 1;

    short* pFpsData = (short*)pData;
    int nConnectedHostCount = 0;

    // 연결이 끊긴 노드는 0으로 초기화
    for(int i = 0; i < MAXCHCNT; i++) {
        if( m_VPSSvr.FindHost(i + 1) == NULL ) {
            m_byOldVideoFpsStatus[i] = VIDEO_FPS_STATUS_NO_CHANGE;
        } else {
            nConnectedHostCount++;
        }
    }

    // 연결된 Host가 없으면 FPS를 보낼 의미가 없음
    if(nConnectedHostCount == 0) return false;

    // 실제 DC로 전송하는 데이터는 FPS 값이 아닌 VPSDEO_FPS_STATUS 값이므로
    // FPS 값을 체크해서 새로 보낼 VIDEO_FPS_STATUS 데이터를 만든다.
    bool hasStatusChanged = false;
    BYTE byNewFpsStatus[MAXCHCNT] = {0,};

    for(int i = 0; i < MAXCHCNT; i++) {
        // 연결되어 있는 Host에 대해서만 체크
        if( m_VPSSvr.FindHost(i + 1) != NULL ) {
            byNewFpsStatus[i] = BASE_MIRRORING_VIDEO_COUNT_PER_10SEC > pFpsData[i] ? VIDEO_FPS_STATUS_FAILURE : VIDEO_FPS_STATUS_SUCCESS;

            // 기존에 보냈을 때와 상황이 다르면(기존에는 에러였는데, 지금은 정상이면(그 역도 마찬가지))
            // 하나라도 상태가 달라지면, hasStatusChanged 를 true로 설정
            if(m_byOldVideoFpsStatus[i] != byNewFpsStatus[i]) {
                hasStatusChanged = true;
            }
        }
    }

    // 정상/에러 상태의 변경이 없으면 안보내도 됨
    if(!hasStatusChanged) return false;

    int totLen = 0;
    BYTE* pSendData = MakeSendData2(CMD_MONITOR_VIDEO_FPS_STATUS, 0, MAXCHCNT, byNewFpsStatus, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
    if( SendToMobileController(pSendData, totLen) ) {
        char log[128] = {0,};
        sprintf(log, "%d: 영상 전송 상태(", CMD_MONITOR_VIDEO_FPS_STATUS);
        for(int i = 0; i < MAXCHCNT; i++) {
            m_byOldVideoFpsStatus[i] = byNewFpsStatus[i];
            if(i != MAXCHCNT - 1) {
                sprintf(log, "%s%d, ", log, byNewFpsStatus[i]);
            } else {
                sprintf(log, "%s%d) 보냄", log, byNewFpsStatus[i]);
            }
        }
        
        AddLog(0, log, LOG_TO_BOTH);   
    }

    return true;
}

void NetManager::CleanupClient(int nHpNo) {
    m_mirror.StopMirroring(nHpNo);
}

bool NetManager::ClosingClient(ClientObject* pClient) {
    if(pClient->m_nClientType == CLIENT_TYPE_HOST) {
        // Guest에 CMD_DISCONNECT_GUEST 보냄
        int nHpNo = pClient->m_nHpNo;
        ClientObject** pClientList = m_VPSSvr.GetClientList(nHpNo);

        for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
            ClientObject* pGuest = pClientList[i];
            if(pGuest != NULL
                && pGuest->m_nClientType == CLIENT_TYPE_GUEST) {
                pGuest->m_isExitCommandReceived = true;
                int totLen = 0;
                BYTE* pSendData = MakeSendData2(CMD_DISCONNECT_GUEST, nHpNo, strlen(pGuest->m_strID), (BYTE*)pGuest->m_strID, gs_pBufSendDataToClient[nHpNo - 1], totLen);
                Send(pSendData, totLen, pGuest, false);
            }
        }
    } else if(pClient->m_nClientType == CLIENT_TYPE_MONITOR) {
        // CMD_MIRRORING_STOPPED 보냄
        int totLen = 0;
        BYTE* pSendData = MakeSendData2(CMD_MIRRORING_STOPPED, pClient->m_nHpNo, 0, NULL, (BYTE*)gs_pBufSendDataToClient[MAXCHCNT], totLen);
        if( SendToMobileController(pSendData, totLen, true) ) {
            AddLog(pClient->m_nHpNo, "DC로 모니터 연결 해제 패킷(22002) 보냄", LOG_TO_BOTH);
        } else {
            AddLog(pClient->m_nHpNo, "DC로 모니터 연결 해제 패킷(22002) 보냄 실패", LOG_TO_BOTH);
        }
    }

    ClientDisconnected(pClient);

    return true;
}

void NetManager::StopVideoServiceIfNoClientExist() {
    // Host 등의 클라이언트는 이미 종료했는데, CMD_START(1) 패킷이 나중에 들어오면,
    // Mirroring 서비스가 종료되지 않는 경우가 발생하여,
    // 해당 채널에 연결된 클라이언트가 하나도 없을 경우 Mirroring 서비스를 종료하는 코드 추가
    bool doesClientExist[MAXCHCNT] = {0,};
    
    for(int i = 0; i < MAXCHCNT; i++) {
        ClientObject** pClientList = m_VPSSvr.GetClientList(i + 1);

        for(int j = 0; j < MAXCLIENT_PER_CH; j++) {
            ClientObject* pClient = pClientList[j];
            if(pClient != NULL
                && !doesClientExist[i] 
                && pClient->m_clientSock != INVALID_SOCKET) {
                doesClientExist[i] = true;
            }
        }

        if( !doesClientExist[i] && !IsOnService(i + 1) ) {
            m_mirror.StopMirroring(i + 1);
        }
    }
}

bool NetManager::CloseClientManualEx(int nHpNo) {
    // 이 함수의 인자가 nHpNo 하나밖에 없으므로,
    // nHpNo 값만 보고 무조건 클라이언트 연결을 종료하면,
    // 클라이언트 리스트 상 가장 먼저 있는 클라이언트를 종료하게 된다.
    // 그래서 Host만으로 제한한다.
    // DC에서는 Host가 종료할때 CMD_STOP(2) 패킷을 보내기 때문이다.
    // 그리고 Host는 VPS에 하나만 연결될 수 있도록 되어 있다.
    ClientObject* pHost = m_VPSSvr.FindHost(nHpNo);
    if(pHost != NULL) {
        CloseClientManual(pHost);
    }

    StopVideoServiceIfNoClientExist();

    // CMD_STOP(2)에 대한 처리 중 하나로, Host가 아닌, 채널 종료에 대한 메시지 처리함
    // TestAutomation에서는 Host가 연결되지 않기 때문에 ClosingClient가 아닌 여기로 옮김
    int nRcvd1 = m_nCaptureCommandReceivedCount[nHpNo - 1];
    int nSent = m_nCaptureCompletionCountSent[nHpNo - 1];

    char log[128] = {0,};
    sprintf(log, "[Capture:Summary] 캡쳐 요청=%d번, 캡쳐 응답=%d번 (%s)", 
                nRcvd1, nSent, (nRcvd1 == nSent) ? "Succeeded" : "Failed");
    AddLog(nHpNo, log, LOG_TO_BOTH);  

    nRcvd1 = m_nRecordStartCommandCountReceived[nHpNo - 1];
    int nRcvd2 = m_nRecordStopCommandCountReceived[nHpNo - 1];
    nSent = m_nRecordCompletionCountSent[nHpNo - 1];

    sprintf(log, "[Record:Summary] 녹화 시작 요청=%d번, 녹화 정지 요청=%d번, 녹화 정지 응답=%d번 (%s)", 
                nRcvd1, nRcvd2, nSent, (nRcvd1 == nRcvd2 && nRcvd2 == nSent) ? "Succeeded" : "Failed");
    AddLog(nHpNo, log, LOG_TO_BOTH);  

    m_nCaptureCommandReceivedCount[nHpNo - 1] = 0;
    m_nCaptureCompletionCountSent[nHpNo - 1] = 0;

    m_nRecordStartCommandCountReceived[nHpNo - 1] = 0;
    m_nRecordStopCommandCountReceived[nHpNo - 1] = 0;
    m_nRecordCompletionCountSent[nHpNo - 1] = 0;

    return true;
}

void NetManager::CloseClientManual(ClientObject* pClient) {
    m_VPSSvr.OnClose(pClient->m_clientSock);
}

bool NetManager::WebCommandDataParsing2(ClientObject* pClient, char* pRcvData, int len) {
    int iDataLen = SwapEndianU4( *(int*)&pRcvData[1] );
    short usCmd = SwapEndianU2( *(short*)&pRcvData[5] );
    int nHpNo = (int)pRcvData[7];

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
            pClient->m_isExitCommandReceived = true;

            int nClientType = pClient->m_nClientType;

            CloseClientManual(pClient);

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
            short width = SwapEndianU2( *(short*)pData );
            short height = SwapEndianU2( *(short*)(pData + 2) );

            m_isOnService[nHpNo - 1] = true;

            char log[128] = {0,};
            sprintf(log, "Start Command 명령 받음: LCD Width=%d, Height=%d", width, height);
            AddLog(nHpNo, log, LOG_TO_BOTH);  

            // start mirroring
            m_mirror.StartMirroring(nHpNo, width, height, DoMirrorCallback, DoMirrorStoppedCallback);

            // 기존 VPS에서는 CMD_PLAYER_QUALITY 명령을 보낸다.
            // 안드로이드 영상 프로세스(nxptc)는 기본 70으로 되어 있어서 보낼 필요 없음
            // ios 영상 프로세스(onycap)는 확인 필요, 우선 처리 보류

            if(m_fnCallback != NULL) {
                CALLBACK cb;

                cb.nHpNo = nHpNo;
                cb.type = CALLBACK_TYPE_DEVICE_START;
                sprintf(cb.data, "%d/%d", width, height);

                m_fnCallback(&cb);
            }
        }
        break;

        case CMD_STOP: {
            bool doesHostExist = m_VPSSvr.FindHost(nHpNo) == NULL ? false : true;
            bool force = true;

            if(doesHostExist) {
                force = false;
            }

            AddLog(nHpNo, "Stop Command 명령 받음 : 채널 서비스 종료 처리 수행", LOG_TO_BOTH);  

            // 캡쳐 중/녹화 중이면, VPS가 DC로 응답하기 전
            // VPS_CAPTURE_RESPONSE_WAITING_TIME 만큼 기다린다.
            if(m_isJpgCapture[nHpNo - 1] || m_isRunRecord[nHpNo -1]) {
                ULONGLONG nTimeout = VPS_CAPTURE_RESPONSE_WAITING_TIME;
                nTimeout += GetTickCount();
                while( (m_isJpgCapture[nHpNo - 1] || m_isRunRecord[nHpNo - 1])
                    && GetTickCount() < nTimeout ) {
                    usleep(1);
                }

                m_isJpgCapture[nHpNo - 1] = false;
                Record(nHpNo, false);
            }

            // force가 true이면 CloseClientManualEx 호출
            if(force) {
                CloseClientManualEx(nHpNo);
            }

            if(force) {
                m_isOnService[nHpNo - 1] = false;
            }

            // isOnService가 변한 시점인 여기에서
            // 한번 더 호출 해준다.
            StopVideoServiceIfNoClientExist();

            if(m_fnCallback != NULL) {
                CALLBACK cb;

                cb.nHpNo = nHpNo;
                cb.type = CALLBACK_TYPE_DEVICE_STOP;

                m_fnCallback(&cb);
            }
        }
        break;

        case CMD_VERTICAL: {
            DeviceRotate(nHpNo, true);
        }
        break;

        case CMD_HORIZONTAL: {
            DeviceRotate(nHpNo, false);
        }
        break;

        case CMD_GIF:
        case CMD_RECORD: {
            bool start = pData[0] == 1 ? true : false;

            if(start) {
                m_nRecordStartCommandCountReceived[nHpNo - 1]++;
            } else {
                m_nRecordStopCommandCountReceived[nHpNo - 1]++;
            }

            // 녹화 명령 처리
            Record(nHpNo, start);

            // DC로부터 Recording Start/Stop 명령을 짝으로 받았는지 체크
            // VPS에서 30분이 지난 다음 Stop 명령이 나중에 날아왔을 때에도
            // 정상적인 응답을 주기 위해 사용함
            m_isReceivedRecordStartCommand[nHpNo - 1] = start;
        }
        break;

        case CMD_JPG_CAPTURE: {
            m_nCaptureCommandReceivedCount[nHpNo - 1]++;

            m_isJpgCapture[nHpNo - 1] = true;
            m_nJpgCaptureStartTime[nHpNo - 1] = GetTickCount();

            AddLog(nHpNo, "[Capture] 단말기 화면 캡쳐 명령 받음", LOG_TO_BOTH);  

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

        case CMD_CAPTURE_COMPLETED: {
            if(m_isReceivedRecordStartCommand[nHpNo - 1]) {
                // VPS 자체적으로 종료한 경우(33분) DC로 알리지 않는다.
                // 추후 Recording Stop을 DC로 부터 받으면 그때 알려준다.
                // file name을 저장해 둔다. : gs_recordFileName[nHpNo - 1]
            } else {
                RecordStopAndSend(nHpNo);
            }
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
            // fps, jpg capture
            for(int i = 0; i < MAXCHCNT; i++) {
                m_iCaptureCountOld[i] = m_iCaptureCount[i];
                m_iCompressedCountOld[i] = m_iCompressedCount[i];
                m_iSendCountOld[i] = m_iSendCount[i];
                m_iCaptureCount[i] = 0;
                m_iCompressedCount[i] = 0;
                m_iSendCount[i] = 0;

                if(m_isJpgCapture[i]
                    && GetTickCount() - m_nJpgCaptureStartTime[i] > VPS_CAPTURE_RESPONSE_WAITING_TIME) {
                    // 에러와 함께 캡쳐 응답 보냄
                    JPGCaptureFailSend(i + 1);

                    m_isJpgCapture[i] = false;
                    m_nJpgCaptureStartTime[i] = 0;
                }

                if(m_isOnService[i] && m_fnCallback != NULL) {
                    CALLBACK cb;

                    cb.nHpNo = i + 1;
                    cb.type = CALLBACK_TYPE_UPDATE_FPS;
                    sprintf(cb.data, "%d/%d/%d", m_iCaptureCountOld[i], m_iCompressedCountOld[i], m_iSendCountOld[i]);

                    m_fnCallback(&cb);
                }

                // 전송량은 host 또는 monitor에 대해서만 표시함
                ClientObject* pHost = m_VPSSvr.FindHost(i + 1);
                ClientObject* pMonitor = m_VPSSvr.FindMonitor(i + 1);
                int sendBytes = 0;
                if(pHost != NULL) {
                    sendBytes = pHost->m_sendBytes;
                    pHost->m_sendBytes = 0;
                } else if(pMonitor != NULL) {
                    sendBytes = pMonitor->m_sendBytes;
                    pMonitor->m_sendBytes = 0;
                }

                if(sendBytes != 0 && m_fnCallback != NULL) {
                    CALLBACK cb;

                    cb.nHpNo = i + 1;
                    cb.type = CALLBACK_TYPE_UPDATE_TRANSFER;
                    sprintf(cb.data, "%d", sendBytes);

                    m_fnCallback(&cb);
                }
            }

            // Record 예외 처리
            for(int i = 0; i < MAXCHCNT; i++) {
                if(m_isRunRecord[i]) {
                    ULONGLONG nElapsedTime = GetTickCount() - m_nRecStartTime[i];

                    // 처음에는 30초 더 기다렸으나, 자동화툴에서는 부족하여 충분하게 3분으로 늘림(33분)
                    if( nElapsedTime >= (RECORDING_TIME + (3 * 60 * 1000)) ) {
                        // 클라이언트로 녹화 중지 명령을 보낸 후,
                        // 3분(33분 - 30분)동안 녹화 중지 명령이 안오면, VPS 자체적으로 녹화 중지함
                        // 자동화 시에는 녹화 중지 명령을 받을 클라이언트가 없으므로, 33분에 녹화 중지됨
                        Record(i + 1, false);
                    } else if(nElapsedTime >= RECORDING_TIME) {
                        // 녹화 시간 30분 경과하면, 클라이언트로 녹화 중지 명령 보냄
                        RecordStopSend(i + 1);
                    }
                } 
            }
        }
        break;

        case TIMERID_10SEC: {
            HeartbeatSendToDC();
        }
        break;

        case TIMERID_15SEC: {
            bool isPerformaingOverloadOperation = false;

            // 로그 파일 Flush 연산은 성능과 관련된 연산이므로
            // 캡쳐나 녹화와 같은 추가 연산을 수행 중이면, Flush 연산을 수행하지 않는다.
            if( m_logger.IsDirty() ) {
                // 녹화 중인 채널과 캡쳐 중인 채널이 없으면
                for(int i = 0; i < MAXCHCNT; i++) {
                    if(m_isRunRecord[i] || m_isJpgCapture[i]) {
                        isPerformaingOverloadOperation = true;
                    }
                }

                // 로그 파일 Flush 연산 수행
                if(!isPerformaingOverloadOperation) {
                    m_logger.Flush();
                }
            }

            if( !isPerformaingOverloadOperation &&
                GetCurrentDay() != m_logger.GetLoggingStartDay() ) {
                m_logger.Restart();
            }
        }
        break;

        case TIMERID_20SEC: {
            // 클라이언트가 연결될 때마다 처음 한번은 장애처리 메시지를 보내지 않는다.
            // 장애처리 패킷에는 모든 채널의 장애 정보가 들어있으므로
            // Client 하나라도 연결이 들어오면, 처음 한번은 장애처리 패킷을 보내지 않는다.
            bool isToSkipFailureReport = false;
            for(int i = 0; i < MAXCHCNT; i++) {
                if(m_isToSkipFailureReport[i]) {
                    m_isToSkipFailureReport[i] = false;
                    isToSkipFailureReport = true;
                }
            }

            if(isToSkipFailureReport) break;

            short nFpsData[MAXCHCNT] = {0,};
            for(int i = 0; i < MAXCHCNT; i++) {
                nFpsData[i] = m_iMirrorVideoInputCount[i];
                m_iMirrorVideoInputCount[i] = 0;
            }

            // 무조건 FPS 값을 보내는 것이 아니라, 보낼지 체크하고 보냄.
            VideoFpsCheckAndSend(nFpsData);
        }
        break;
    }
}

int NetManager::GetLastScene(int nHpNo, BYTE* pDstJpg) {
    int ret = 0;

    if(m_isOnService[nHpNo - 1]) {
        m_lockCurrenScene[nHpNo - 1].Lock();

        memcpy(pDstJpg, gs_lastScene[nHpNo - 1].data, MAX_SIZE_RGB_DATA);
        ret = MAX_SIZE_RGB_DATA;

        m_lockCurrenScene[nHpNo - 1].Unlock();
    }

    return ret;
}

void NetManager::AddLog(int nHpNo, const char* log, vps_log_target_t nTarget) {
    if(m_fnCallback != NULL) {
        if(nTarget == LOG_TO_UI || nTarget == LOG_TO_BOTH) {
            CALLBACK cb;

            cb.nHpNo = nHpNo;
            cb.type = CALLBACK_TYPE_LOG;
            strcpy(cb.data, log);

            m_fnCallback(&cb);
        }
    } 
    
    char prefix[32] = {0,};
    char logText[256] = {0,};

    SYSTEM_TIME stTime;
    GetLocalTime(stTime);

    sprintf(prefix, "[%04d%02d%02d %02d:%02d:%02d:%03d]", 
                        stTime.year, stTime.month, stTime.day, 
                        stTime.hour, stTime.minute, stTime.second, stTime.millisecond);

    sprintf(logText, "[VPS:%d] %s\n", nHpNo, log);

    if(nTarget == LOG_TO_UI || nTarget == LOG_TO_BOTH) {
        printf("%s", prefix);
        printf(" %s", logText);
    }

    if(nTarget == LOG_TO_FILE || nTarget == LOG_TO_BOTH) {
        m_logger.AddLog(logText);
    }
}