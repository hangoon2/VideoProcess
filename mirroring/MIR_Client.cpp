#include "MIR_Client.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <algorithm>

using namespace std;

#if ENABLE_NONBLOCK_SOCKET
const int kReadEvent = 1;
#endif

bool SetNonBlock(Socket sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if(flags >= 0) {
        int r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        if(r >= 0) {
            return true;
        }
    }

    return false;
}

void* ThreadFunc(void* pArg) {
    MIR_Client *pClient = (MIR_Client*)pArg;
    int nMirrorPort = pClient->GetMirrorPort();
    int nControlPort = pClient->GetControlPort();

    pClient->SetIsThreadRunning(true);

    /* ///////////////////////////////////////////////////
    //                Mirroring Socket                  //
    /////////////////////////////////////////////////// */
    pClient->m_mirrorSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(pClient->m_mirrorSocket == -1) {
        char log[128] = {0,};
        sprintf(log, "mirroring sock() error[%d]", errno);
        pClient->AddLog(log, LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }

    // 종료방식
    // 즉시 연결을 종료한다. 상대방에게는 FIN이나 RTS 시그널이 전달된다.
    short l_onoff = 1, l_linger = 0;    
    linger opt = {l_onoff, l_linger};
    int state = setsockopt( pClient->m_mirrorSocket, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt) );
    if(state) {
        pClient->AddLog("mirroring socket setsockopt() SO_LINGER error", LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }

    struct sockaddr_in servAddr;
    memset( &servAddr, 0x00, sizeof(servAddr) );
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(nMirrorPort);

    if( connect( pClient->m_mirrorSocket, (sockaddr*)&servAddr, sizeof(servAddr) ) != 0 ) {
        char log[128] = {0,};
        sprintf(log, "mirroring socket connect() error[%d]", errno);
        pClient->AddLog(log, LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }
    
    // send: 'sendme'
    write( pClient->m_mirrorSocket, "sendme", sizeof("sendme") );

    /* ///////////////////////////////////////////////////
    //                 Control Socket                   //
    /////////////////////////////////////////////////// */
    pClient->m_controlSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(pClient->m_controlSocket == -1) {
        char log[128] = {0,};
        sprintf(log, "control sock() error[%d]", errno);
        pClient->AddLog(log, LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }

    state = setsockopt( pClient->m_controlSocket, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt) );
    if(state) {
        pClient->AddLog("control socket setsockopt() SO_LINGER error", LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }

    memset( &servAddr, 0x00, sizeof(servAddr) );
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(nControlPort);

    if( connect( pClient->m_controlSocket, (sockaddr*)&servAddr, sizeof(servAddr) ) != 0 ) {
        char log[128] = {0,};
        sprintf(log, "control socket connect() error[%d]", errno);
        pClient->AddLog(log, LOG_TO_FILE);

        pClient->CleanUpRunClientThreadData();

        pClient->OnMirrorStopped(-1);

        pClient->SetIsThreadRunning(false);

        return NULL;
    }

    // send : mirror on
    pClient->SendOnOffPacket(true);

    pClient->SetRunClientThreadReady(true);
    pClient->SetDoExitRunClientThread(false);

#if ENABLE_NONBLOCK_SOCKET
    int epollfd = kqueue();
    if(epollfd >= 0) {
        SetNonBlock(pClient->m_mirrorSocket);

        struct kevent ev[2];
        int n = 0;
        
        EV_SET(&ev[n++], pClient->m_mirrorSocket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)pClient->m_mirrorSocket);

        kevent(epollfd, ev, n, NULL, 0, NULL);

        // memory alloc : read buffer
        pClient->m_pRcvBuf = (BYTE*)malloc(RECV_BUF_SIZE);

        while( !pClient->IsDoExitRunClientThread() ) {
            if( !pClient->GetData(epollfd, pClient->m_mirrorSocket, 1000) ) {
                pClient->SetDoExitRunClientThread(true);

                pClient->AddLog("Mirroring socket is disconnected.", LOG_TO_FILE);
                break;
            }
        }

        free(pClient->m_pRcvBuf);
        pClient->m_pRcvBuf = NULL;
    }
#else
    // memory alloc : read buffer
    pClient->m_pRcvBuf = (BYTE*)malloc(RECV_BUF_SIZE);

    while( !pClient->IsDoExitRunClientThread() ) {
        int ret = pClient->GetData();
        if(ret == -1 || ret == 0) {
            pClient->SetDoExitRunClientThread(true);

            pClient->AddLog("Mirroring socket is disconnected.", LOG_TO_FILE);
            break;
        }
    }

    free(pClient->m_pRcvBuf);
    pClient->m_pRcvBuf = NULL;
#endif

    pClient->SetRunClientThreadReady(false);
    pClient->AddLog("Mirroring service stopped", LOG_TO_BOTH);

    // Stop 처리에서 이미 보냄
//    // send : mirror off
//    pClient->SendOnOffPacket(false);

    pClient->CleanUpRunClientThreadData();

    pClient->OnMirrorStopped(0);

    pClient->SetIsThreadRunning(false);

    return NULL;
}

MIR_Client::MIR_Client() {
    m_pMirroringRoutine = NULL;
    m_pMirroringStopRoutine = NULL;
    m_pLogRoutine = NULL;

    m_tID = NULL;

    m_isThreadRunning = false;
    m_doExitRunClientThread = false;
    m_isRunClientThreadReady = false;

    m_mirrorSocket = -1;
    m_controlSocket = -1;

    m_pRcvBuf = NULL;

    m_pos = 0;
    m_rxStreamOrder = RX_PACKET_POS_START;
    m_dataSize = 0;

    m_nOffset = 0;
    m_nCurrReadSize = 0;
    m_nReadBufSize = 0;
    m_isHeadOfFrame = true;
    m_isFirstImage = true;
}

MIR_Client::~MIR_Client() {

}

bool MIR_Client::StartRunClientThread(int nHpNo, int nMirroringPort, int nControlPort, 
                                        PMIRRORING_ROUTINE pMirroringRoutine, PMIRRORING_STOP_ROUTINE pMirroringStopRoutine) {
    bool ret = false;

    if( !IsThreadRunning() ) {
        m_nHpNo = nHpNo;
        m_nMirrorPort = nMirroringPort;
        m_nControlPort = nControlPort;

        m_pMirroringRoutine = pMirroringRoutine;
        m_pMirroringStopRoutine = pMirroringStopRoutine;

        SetRunClientThreadReady(false);

        pthread_create(&m_tID, NULL, &ThreadFunc, this);
        if(m_tID != NULL) {
            AddLog("Mirroring thread create success", LOG_TO_FILE);

            ret = true;
        } else {
            char log[128] = {0,};
            sprintf(log, "Mirroring thread creation fail[%d]", errno);
            AddLog(log, LOG_TO_FILE);
        }
    } else {
        AddLog("Already mirroring thread", LOG_TO_FILE);

        // send key frame
        SendKeyFramePacket();
    }

    return ret;
}

void MIR_Client::StopRunClientThread() {
    if( m_tID != NULL || IsThreadRunning() ) {
        // 미러링 off 패킷을 보내지 않고 mirroring이 종료되면 <crash>가 발생하므로
        // 미러링 소켓을 닫기전에 보냄
        SendOnOffPacket(false);

        CleanUpRunClientThreadData();
    }
}

void MIR_Client::SetAddLogLisnter(PVPS_ADD_LOG_ROUTINE pLogRoutine) {
    m_pLogRoutine = pLogRoutine;
}

void MIR_Client::OnMirrorStopped(int nStopCode) {
    if(m_pMirroringStopRoutine != NULL) {
        m_pMirroringStopRoutine(m_nHpNo, nStopCode);
    }
}

void MIR_Client::SetIsThreadRunning(bool isThreadRunning) {
    m_isThreadRunning = isThreadRunning;
}

bool MIR_Client::IsThreadRunning() {
    return m_isThreadRunning;
}

void MIR_Client::SetDoExitRunClientThread(bool doExitRunClientThread) {
    m_doExitRunClientThread = doExitRunClientThread;
}

bool MIR_Client::IsDoExitRunClientThread() {
    return m_doExitRunClientThread;
}

void MIR_Client::SetRunClientThreadReady(bool isRunClientThreadReady) {
    m_isRunClientThreadReady = isRunClientThreadReady;
}

bool MIR_Client::IsRunClientThreadReady() {
    return m_isRunClientThreadReady;
}

void MIR_Client::CleanUpRunClientThreadData() {
    if(m_tID != NULL) {
        m_tID = NULL;

        SetDoExitRunClientThread(false);

        if(m_mirrorSocket != -1) {
            AddLog("Mirroring Socket closed", LOG_TO_FILE);

            shutdown(m_mirrorSocket, SHUT_RDWR);
            close(m_mirrorSocket);
            m_mirrorSocket = -1;
        }

        if(m_controlSocket != -1) {
            AddLog("Control Socket closed", LOG_TO_FILE);

            shutdown(m_controlSocket, SHUT_RDWR);
            close(m_controlSocket);
            m_controlSocket = -1;
        }
    }
}

int MIR_Client::GetHpNo() {
    return m_nHpNo;
}

int MIR_Client::GetMirrorPort() {
    return m_nMirrorPort;
}

int MIR_Client::GetControlPort() {
    return m_nControlPort;
}

int MIR_Client::SendToControlSocket(const char* buf, int len) {
    if(m_controlSocket != -1) {
        int ret = (int)write(m_controlSocket, buf, len);
        return ret;
    }

    return 0;
}

int MIR_Client::SendOnOffPacket(bool onoff) {
    if(m_controlSocket != -1) {
        int size = 0;
        BYTE* ptrData = MakeOnyPacketOnOff(onoff, size);
        if(size != 0) {
            return SendToControlSocket((const char*)ptrData, size);
        }
    }

    return 0;
}

int MIR_Client::SendKeyFramePacket() {
    if(m_controlSocket != -1) {
        int size = 0;
        BYTE* ptrData = MakeOnyPacketKeyFrame(size);
        if(size != 0) {
            return SendToControlSocket((const char*)ptrData, size);
        }
    }

    return 0;
}

BYTE* MIR_Client::MakeOnyPacketKeyFrame(int& size) {
    int dataSum = 0;
    int sizeofData = 0;

    memset(m_sendBuf, 0x00, SEND_BUF_SIZE);

    BYTE mStartFlag = CMD_START_CODE;
    sizeofData = sizeof(mStartFlag);
    memcpy(m_sendBuf, (char*)&mStartFlag, sizeofData);
    dataSum = sizeofData;

    INT mDataSize = htonl(0);
    sizeofData = sizeof(mDataSize);
    memcpy(m_sendBuf + dataSum, (char*)&mDataSize, sizeofData);
    dataSum += sizeofData;

    UINT16 mCommandCode = htons(CMD_KEY_FRAME);
    sizeofData = sizeof(mCommandCode);
    memcpy(m_sendBuf + dataSum, (char*)&mCommandCode, sizeofData);
    dataSum += sizeofData;

    BYTE mDeviceNo = (BYTE)m_nHpNo;
    sizeofData = sizeof(mDeviceNo);
    memcpy(m_sendBuf + dataSum, (char*)&mDeviceNo, sizeofData);
    dataSum += sizeofData;

    UINT16 mChecksum = htons( CalChecksum((UINT16*)(m_sendBuf + 1), ntohl(mDataSize) + CMD_HEAD_SIZE - 1) );
    sizeofData = sizeof(mChecksum);
    memcpy(m_sendBuf + dataSum, (char*)&mChecksum, sizeofData);
    dataSum += sizeofData;

    BYTE mEndFlag = CMD_END_CODE;
    sizeofData = sizeof(mEndFlag);
    memcpy(m_sendBuf + dataSum, (char*)&mEndFlag, sizeofData);
    dataSum += sizeofData;

    size = dataSum;

    return m_sendBuf;
}

BYTE* MIR_Client::MakeOnyPacketOnOff(bool onoff, int& size) {
    int dataSum = 0;
    int sizeofData = 0;

    memset(m_sendBuf, 0x00, SEND_BUF_SIZE);

    BYTE mStartFlag = CMD_START_CODE;
    sizeofData = sizeof(mStartFlag);
    memcpy(m_sendBuf, (char*)&mStartFlag, sizeofData);
    dataSum = sizeofData;

    INT mDataSize = htonl(1);
    sizeofData = sizeof(mDataSize);
    memcpy(m_sendBuf + dataSum, (char*)&mDataSize, sizeofData);
    dataSum += sizeofData;

    UINT16 mCommandCode = htons(CMD_ON_OFF);
    sizeofData = sizeof(mCommandCode);
    memcpy(m_sendBuf + dataSum, (char*)&mCommandCode, sizeofData);
    dataSum += sizeofData;

    BYTE mDeviceNo = (BYTE)m_nHpNo;
    sizeofData = sizeof(mDeviceNo);
    memcpy(m_sendBuf + dataSum, (char*)&mDeviceNo, sizeofData);
    dataSum += sizeofData;

    BYTE mOnOff = onoff ? 1 : 0;
    sizeofData = sizeof(mOnOff);
    memcpy(m_sendBuf + dataSum, (char*)&mOnOff, sizeofData);
    dataSum += sizeofData;

    UINT16 mChecksum = htons( CalChecksum((UINT16*)(m_sendBuf + 1), ntohl(mDataSize) + CMD_HEAD_SIZE - 1) );
    sizeofData = sizeof(mChecksum);
    memcpy(m_sendBuf + dataSum, (char*)&mChecksum, sizeofData);
    dataSum += sizeofData;

    BYTE mEndFlag = CMD_END_CODE;
    sizeofData = sizeof(mEndFlag);
    memcpy(m_sendBuf + dataSum, (char*)&mEndFlag, sizeofData);
    dataSum += sizeofData;

    size = dataSum;

    return m_sendBuf;
}

#if ENABLE_NONBLOCK_SOCKET
bool MIR_Client::GetData(int efd, Socket sock, int waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);

    for(int i = 0; i < n; i++) {
        int events = activeEvs[i].filter;

        if(events == EVFILT_READ) {
            char buf[4096];
            int readLen = 0; 

            while( (readLen = ::read(sock, buf, sizeof buf)) > 0 ) {
                int idx = 0;
                int iWrite = 0;
                while(idx < readLen) {
                    if(m_rxStreamOrder == RX_PACKET_POS_START) {
                        // find start flag
                        for(; idx < readLen; ++idx) {
                            if(buf[idx] == CMD_START_CODE) {
                                m_rxStreamOrder = RX_PACKET_POS_HEAD;
                                m_pos = 0;
                                break;
                            }
                        }
                    }

                    if(m_rxStreamOrder == RX_PACKET_POS_HEAD) {
                        iWrite = min(CMD_HEAD_SIZE - m_pos, readLen - idx);

                        memcpy(m_pRcvBuf + m_pos, buf + idx, iWrite);

                        m_pos += iWrite;
                        idx += iWrite;

                        if(m_pos == CMD_HEAD_SIZE) {
                            m_dataSize = ntohl( *((__int32_t*)(m_pRcvBuf + 1)) );

                            m_pos = 0;
                            m_rxStreamOrder = RX_PACKET_POS_DATA;
                        }
                    }

                    if(m_rxStreamOrder == RX_PACKET_POS_DATA) {
                        iWrite = min(m_dataSize - m_pos, readLen - idx);

                        memcpy(m_pRcvBuf + CMD_HEAD_SIZE + m_pos, buf + idx, iWrite);

                        m_pos += iWrite;
                        idx += iWrite;

                        if(m_pos == m_dataSize) {
                            m_pos = 0;
                            m_rxStreamOrder = RX_PACKET_POS_TAIL;
                        }
                    }

                    if(m_rxStreamOrder == RX_PACKET_POS_TAIL) {
                        iWrite = min(CMD_TAIL_SIZE - m_pos, readLen - i);

                        memcpy(m_pRcvBuf + CMD_HEAD_SIZE + m_dataSize + m_pos, buf + idx, iWrite);

                        m_pos += iWrite;
                        idx += iWrite;

                        if(m_pos == CMD_TAIL_SIZE) {
                            m_rxStreamOrder = RX_PACKET_POS_START;

                            if(m_pMirroringRoutine != NULL) {
                                m_pMirroringRoutine( (void*)m_pRcvBuf );
                            }
                        }
                    }
                }
            }

            if( readLen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
                continue;
            }

            return false;
        } 
    }

    return true;
}
#endif

int MIR_Client::GetData() {
    int r = -2;

    if(m_isHeadOfFrame) {
        r = m_nCurrReadSize = recv(m_mirrorSocket, m_pRcvBuf + m_nOffset, 4, 0);
        if(m_nCurrReadSize == -1) {
            return r;
        } else if(m_nCurrReadSize == 0) {
            AddLog("Read Error : gracefully closed.", LOG_TO_FILE);
            return r;
        } 

        m_nOffset = m_nOffset + m_nCurrReadSize;

        if(m_nOffset >= CMD_HEAD_SIZE) {
            int dataSize = ntohl(*((__int32_t*)(m_pRcvBuf + 1)));
            m_nReadBufSize = dataSize - (m_nOffset - CMD_HEAD_SIZE) + CMD_TAIL_SIZE;
            m_isHeadOfFrame = false;
        }
    } else {
        // 헤더에서 계산된 data size 만큼
        r = m_nCurrReadSize = recv(m_mirrorSocket, m_pRcvBuf + m_nOffset, m_nReadBufSize, 0);
        if(m_nCurrReadSize <= 0) {
            return r;
        }

        m_nReadBufSize = m_nReadBufSize - m_nCurrReadSize;
        m_nOffset = m_nOffset + m_nCurrReadSize;
        if(m_nReadBufSize == 0) {
            m_nOffset = 0;

            int dataSize = ntohl(*((__int32_t*)(m_pRcvBuf + 1)));
            int iPacketLen = CMD_HEAD_SIZE + dataSize + CMD_TAIL_SIZE;
            if(RECV_BUF_SIZE < iPacketLen) {
                printf("[VPS:%d] Read Error : Too many data\n", m_nHpNo);
                return -4;
            }

            if(((BYTE*)m_pRcvBuf)[iPacketLen - 1] != CMD_END_CODE) {
                printf("[VPS:%d] Read Error : Invalid packet\n", m_nHpNo);
                return -3;
            }

            if(m_pMirroringRoutine != NULL) {
                m_pMirroringRoutine( (void*)m_pRcvBuf );
            }

            m_isHeadOfFrame = true;
        }
    }

    return m_nCurrReadSize;
}

void MIR_Client::AddLog(const char* log, vps_log_target_t nTarget) {
    if(m_pLogRoutine != NULL) {
        m_pLogRoutine(m_nHpNo, log, nTarget);
    }
}