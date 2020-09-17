#include "AsyncMediaServerSocket.h"
#include "NetManager.h"

#include <sys/socket.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

const int kReadEvent = 1;
//const int kWriteEvent = 2;

static int gs_pid = -1;

AsyncMediaServerSocket::AsyncMediaServerSocket() {
    m_serverSock = INVALID_SOCKET;

    m_pNetMgr = NULL;

    m_isRunServer = true;
    m_isClosed = false;
}

AsyncMediaServerSocket::~AsyncMediaServerSocket() {
    m_isRunServer = false;

    if(m_serverSock != INVALID_SOCKET) {
        printf("[VPS:0] Closed Listening Server\n");
        UpdateEvents(m_queueID, m_serverSock, kReadEvent, true);

        shutdown(m_serverSock, SHUT_RDWR);
        close(m_serverSock);
        m_serverSock = INVALID_SOCKET;
    }
}

void AsyncMediaServerSocket::SetBlock(Socket sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if(flags < 0) {
        printf("[VPS:0] fcntl failed\n");
        return;
    }

    int r = fcntl(sock, F_SETFL, flags & O_NONBLOCK);
    if(r < 0) {
        printf("[VPS:0] fcntl failed\n");
        return;
    }
}

void AsyncMediaServerSocket::SetNonBlock(Socket sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if(flags < 0) {
        printf("[VPS:0] fcntl failed\n");
        return;
    }

    int r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    if(r < 0) {
        printf("[VPS:0] fcntl failed\n");
        return;
    }
}

void AsyncMediaServerSocket::UpdateEvents(int efd, Socket sock, int events, bool modify) {
    struct kevent ev[1];
    int n = 0;
    
    if(events & kReadEvent) {
        EV_SET(&ev[n], sock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)sock);
    } else if(modify) {
        EV_SET(&ev[n], sock, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)sock);
    }

    // if(events & kWriteEvent) {
    //     EV_SET(&ev[n++], sock, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)sock);
    // } else if(modify) {
    //     EV_SET(&ev[n++], sock, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)sock);
    // }

    // if(events & kExceptEvent) {
    //     EV_SET(&ev[n++], sock, EVFILT_AIO, EV_ADD | EV_ENABLE, 0, 0, (void*)(intptr_t)sock);
    // } else if(modify) {
    //     EV_SET(&ev[n++], sock, EVFILT_AIO, EV_DELETE, 0, 0, (void*)(intptr_t)sock);
    // }

    kevent(efd, ev, 1, NULL, 0, NULL);
}

void AsyncMediaServerSocket::OnAccept(int efd, ServerSocket serverSock) {
    struct sockaddr_in raddr;
    memset( &raddr, 0x00, sizeof(raddr) );
    socklen_t rsz = sizeof(raddr);
    Socket clientSock = accept(serverSock, (struct sockaddr *)&raddr, &rsz);
    if(clientSock < 0) {
        printf("[VPS:0] accept failed ==> error no: %d error msg %s\n", errno, strerror(errno));
        return;
    }

    struct sockaddr_in peer;
    memset( &peer, 0x00, sizeof(peer) );
    socklen_t alen = sizeof(peer);
    int r = getpeername(clientSock, (sockaddr *)&peer,  &alen);
    if(r < 0) {
        printf("[VPS:0] getpeername failed ==> error no: %d error msg %s\n", errno, strerror(errno));
        return;
    }

    SetNonBlock(clientSock);
    UpdateEvents(efd, clientSock, kReadEvent, false);

    ClientObject* pClient = new ClientObject();
    pClient->m_clientSock = clientSock;
    strcpy( pClient->m_strIPAddr, inet_ntoa(raddr.sin_addr) );

    m_clientList.Insert(clientSock, pClient);

    int bufsize = 1024 * 256;
    setsockopt( clientSock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize) );

    // 종료방식
    short l_onOff = 1, l_linger = 0;    // 즉시 연결을 종료한다. 상대방에게는 FIN이나 RTS 시그널이 전달된다.
    linger opt = {l_onOff, l_linger};
    setsockopt( clientSock, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt) );

    // 즉시전송(Nagle 알고리즘 해제)
    int optval = 1;
    setsockopt( clientSock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval) );

    char log[128] = {0,};
    sprintf(log, "Accept : %s", pClient->m_strIPAddr);
    ((NetManager*)m_pNetMgr)->AddLog(0, log, LOG_TO_FILE); 
}

void AsyncMediaServerSocket::OnRead(int efd, Socket sock) {
    char buf[4096];
    int n = 0; 

    while( (n = ::read(sock, buf, sizeof buf)) > 0 ) {
        if(m_pNetMgr != NULL) {
            ((NetManager*)m_pNetMgr)->OnReadEx( m_clientList.Find(sock), buf, n );
        }
    }

    if( n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) ) {
        return;
    }
 
    OnClose(sock);
}

bool AsyncMediaServerSocket::OnClose(Socket sock) {
    ClientObject* pClient = m_clientList.Find(sock);
    if(pClient != NULL) {  
        if(pClient->m_isClosing) return false;

        pClient->m_isClosing = true;

        // close socket
        UpdateEvents(m_queueID, sock, kReadEvent, true);

        shutdown(sock, SHUT_RDWR);
        close(sock);

        char log[128] = {0,};
        sprintf(log, "Socket Closed : %d", sock);
        ((NetManager*)m_pNetMgr)->AddLog(pClient->m_nHpNo, log, LOG_TO_FILE); 

        int nHpNo = pClient->m_nHpNo;
        int nClientType = pClient->m_nClientType;
        bool doesHostExist = FindHost(nHpNo) == NULL ? false : true;
        bool doesMonitorExist = FindMonitor(nHpNo) == NULL ? false : true;
        bool isOnService = ((NetManager*)m_pNetMgr)->IsOnService(nHpNo);

        if(nClientType == CLIENT_TYPE_UNKNOWN) {
            // Video Recorder가 Unknown type으로 연결했다가 종료함
            DeleteClient(pClient);

            return true;
        }

        if( pClient == GetMobileController() ) {
            ((NetManager*)m_pNetMgr)->ClosingClient(pClient);

            DeleteClient(pClient);
            
            return true;
        }

        // if( !pClient->m_isExitCommandReceived && nClientType != CLIENT_TYPE_MONITOR ) {
        //     // 비정상적으로 연결 해제된 Client 정보를 저장하되
        //     // m_SocketClient 변수값은 초기화한다.
        //     printf("========= 비정상 종료 처리 필요 ==========\n");
        //     pClient->m_isClosing = false;

        //     return true;
        // }

        if(nClientType == CLIENT_TYPE_HOST) {
            // 동영상 녹화 정지
            // Host와 TestAutomation에서만 녹화 가능한데,
            // 이 둘은 서로 함께 사용할 수 없으므로, !isOnService 검사는 제거함
            if(!doesHostExist) {
                // 해당 채널 레코드 종료 보냄(자동화)
                ((NetManager*)m_pNetMgr)->Record(nHpNo, false);
            }
            if(!doesHostExist && !doesMonitorExist && !isOnService) {
                ((NetManager*)m_pNetMgr)->CleanupClient(nHpNo);
            }

            ((NetManager*)m_pNetMgr)->ClosingClient(pClient);

//            usleep(100);
            CloseAllGuest(nHpNo);

            DeleteClient(pClient);
        } else if(nClientType == CLIENT_TYPE_MONITOR) {
            // 자동화 테스트 중에는 Monitor 모드의 VD가 닫혀도 Clean Up 처리를 하지 않기 위해 
            // isOnService 체크함
            if(!doesHostExist && !doesMonitorExist && !isOnService) {
                ((NetManager*)m_pNetMgr)->CleanupClient(nHpNo);
            }

            if(!doesMonitorExist) {
                // DC로 모니터 연결해제 패킷 보냄
                ((NetManager*)m_pNetMgr)->ClosingClient(pClient);
            }

            DeleteClient(pClient);
        }
        
        return true;
    }

    return false;
}

bool AsyncMediaServerSocket::OnSend(ClientObject* pClient, BYTE* pData, int iLen, bool force) {
    if(iLen >= MAX_SIZE_JPEG_DATA) {
        printf("패킷 사이즈 너무 큼\n");
        return false;
    }

    if(pClient->m_isClosing && !force) {
        printf("[VPS:%d] 소켓 Close 처리 중\n", pClient->m_nHpNo);
        return false;
    }

    Socket sock = pClient->m_clientSock;

    for(int i = 0; i < 5; i++) {
        int nSendResult = send(sock, pData, iLen, 0);
//        int nSendResult = write(sock, pData, iLen);
        if(nSendResult == SOCKET_ERROR) {
            if(errno == EWOULDBLOCK) {
                usleep(1);
                continue;
            } else {
                printf("[VPS:%d] Socket Send() Error : %d\n", pClient->m_nHpNo, errno);
                return false;
            }
        } else {
            if(nSendResult != iLen) {
                printf("[VPS:%d] SEND DATA : %d/%d (%d)\n", pClient->m_nHpNo, nSendResult, iLen, errno);
                return false;
            }
                
            return true;
        }
    }

    return false;
}

bool AsyncMediaServerSocket::OnServerEvent(int efd, ServerSocket serverSock, int waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);

    for(int i = 0; i < n; i++) {
        Socket clientSock = (int)(intptr_t)activeEvs[i].udata;
        int events = activeEvs[i].filter;

        if(events == EVFILT_READ) {
            if(clientSock == serverSock) {
                OnAccept(efd, clientSock);
            } else {
                OnRead(efd, clientSock);
            }
        } else if(events == EVFILT_WRITE) {
//            printf("-------------------------> EVFILT_WRITE");
        } else if(events == EVFILT_SIGNAL) {
//            printf("========================================> EVFILT_SIGNAL\n");
            return false;
        } else {
            printf("[VPS:0] unknown event\n");
        }
    }

    return true;
}

int AsyncMediaServerSocket::InitSocket(void* pNetMgr, int port) {
    m_pNetMgr = pNetMgr;

    m_queueID = kqueue();
    if(m_queueID < 0) {
        printf("[VPS:0] epoll create failed\n");
        return -1;
    }

    m_serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if(m_serverSock < 0) {
        printf("[VPS:0] server socket failed\n");
        return -1;
    }

    int option = 1;
    setsockopt( m_serverSock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option) );

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int r = ::bind( m_serverSock, (struct sockaddr *)&addr, sizeof(struct sockaddr) );
    if(r) {
        printf("[VPS:0] bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));
        return -1;
    } 

    // 입출력 버퍼크기의 변경
    int send_buf = 1024 * 256;
    int rcv_buf = 1024 * 128;

    setsockopt( m_serverSock, SOL_SOCKET, SO_RCVBUF, &rcv_buf, sizeof(rcv_buf) );
    setsockopt( m_serverSock, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf) );

    // 종료방식
    short l_onOff = 1, l_linger = 0;    // 즉시 연결을 종료한다. 상대방에게는 FIN이나 RTS 시그널이 전달된다.
    linger opt = {l_onOff, l_linger};
    setsockopt( m_serverSock, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt) );

    // 즉시전송(Nagle 알고리즘 해제)
    int optval = 1;
    setsockopt( m_serverSock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval) );

    r = listen(m_serverSock, 300);  // Client(host 1 + guest 20) * device num + Device Controller
    if(r) {
        printf("[VPS:0] listen failed %d %s", errno, strerror(errno));
        return -1;
    }

    char log[128] = {0,};
    sprintf(log, "VPS Server listening : %d", port);
    ((NetManager*)m_pNetMgr)->AddLog(0, log, LOG_TO_FILE); 

    SetNonBlock(m_serverSock);
    UpdateEvents(m_queueID, m_serverSock, kReadEvent, false);

    struct kevent ev;
    EV_SET(&ev, SIGINT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
    signal(SIGINT, SIG_IGN);
    kevent(m_queueID, &ev, 1, NULL, 0, NULL);

    ((NetManager*)m_pNetMgr)->AddLog(0, "VPS 초기화 완료", LOG_TO_BOTH); 

    gs_pid = getpid();

    while(m_isRunServer) {
        if( !OnServerEvent(m_queueID, m_serverSock, 10000) ) {
            break;
        }
    }

    m_isClosed = true;
    printf("SERVER THREAD STOP OK\n");

    return 0;
}

void AsyncMediaServerSocket::DestSocket() {
    ClientObject* pMobileController = m_clientList.GetMobileController();
    if(pMobileController != NULL) {
        OnClose(pMobileController->m_clientSock);
    }

    m_isRunServer = false;
    if(gs_pid != -1) {
        kill(gs_pid, SIGINT);
    }

    if(m_serverSock != INVALID_SOCKET) {
        printf("[VPS:0] Closed Listening Server\n");
        UpdateEvents(m_queueID, m_serverSock, kReadEvent, true);

        shutdown(m_serverSock, SHUT_RDWR);
        close(m_serverSock);
        m_serverSock = INVALID_SOCKET;
    }

    int count = 0;
    while(!m_isClosed) {
        sleep(1);

        count++;

        if(count == 20) break;
    }

    printf("CLOSE SERVER OK\n");
}

ClientObject* AsyncMediaServerSocket::Find(Socket sock) {
    ClientObject* pClient = NULL;

    m_mClientLock.Lock();

    pClient = m_clientList.Find(sock);

    m_mClientLock.Unlock();

    return pClient;
}

ClientObject* AsyncMediaServerSocket::FindHost(int nHpNo) {
    ClientObject* pHost = NULL;

    m_mClientLock.Lock();

    pHost = m_clientList.FindHost(nHpNo);

    m_mClientLock.Unlock();

    return pHost;
}

ClientObject* AsyncMediaServerSocket::FindGuest(int nHpNo, char* id) {
    ClientObject* pGuest = NULL;

    m_mClientLock.Lock();

    pGuest = m_clientList.FindGuest(nHpNo, id);

    m_mClientLock.Unlock();

    return pGuest;
}

ClientObject* AsyncMediaServerSocket::GetMobileController() {
    return m_clientList.GetMobileController();
}

ClientObject* AsyncMediaServerSocket::FindMonitor(int nHpNo) {
    ClientObject* pMonitor = NULL;

    m_mClientLock.Lock();

    pMonitor = m_clientList.FindMonitor(nHpNo);

    m_mClientLock.Unlock();

    return pMonitor;
}

void AsyncMediaServerSocket::UpdateClientList(ClientObject* pClient) {
    m_mClientLock.Lock();

    m_clientList.UpdateClient(pClient);

    m_mClientLock.Unlock();
}

ClientObject** AsyncMediaServerSocket::GetClientList(int nHpNo) {
    ClientObject** pClientList = NULL;

    m_mClientLock.Lock();

    pClientList = m_clientList.GetClientList(nHpNo);

    m_mClientLock.Unlock();

    return pClientList;
}

void AsyncMediaServerSocket::CloseAllGuest(int nHpNo) {
    ClientObject** pClientList = GetClientList(nHpNo);

    for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
        ClientObject* pClient = pClientList[i];
        if(pClient != NULL
            && pClient->m_nClientType == CLIENT_TYPE_GUEST) {
            shutdown(pClient->m_clientSock, SHUT_RDWR);
            close(pClient->m_clientSock);

            m_clientList.Delete(pClient->m_clientSock);

            delete pClient;
            pClient = NULL;
        }
    }
}

void AsyncMediaServerSocket::DeleteClient(ClientObject* pClient) {
    m_clientList.Delete(pClient->m_clientSock);

    delete pClient;
    pClient = NULL;
}