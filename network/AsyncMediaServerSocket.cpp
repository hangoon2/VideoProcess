#include "AsyncMediaServerSocket.h"
#include "NetManager.h"

#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

//#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("\nerror no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

const int kReadEvent = 1;
const int kWriteEvent = 2;

AsyncMediaServerSocket::AsyncMediaServerSocket() {
    m_serverSock = INVALID_SOCKET;

    m_pNetMgr = NULL;

    m_isRunServer = true;
}

AsyncMediaServerSocket::~AsyncMediaServerSocket() {
    m_clientList.Clear();

    m_isRunServer = false;

    if(m_serverSock != INVALID_SOCKET) {
        printf("[VPS:0] Closed Listening Server\n");
        close(m_serverSock);
        m_serverSock = INVALID_SOCKET;
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
    struct kevent ev[2];
    int n = 0;
    
    if(events & kReadEvent) {
        EV_SET(&ev[n++], sock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)sock);
    } else if(modify) {
        EV_SET(&ev[n++], sock, EVFILT_READ, EV_DELETE, 0, 0, (void *)(intptr_t)sock);
    }

    // if(events & kWriteEvent) {
    //     EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(intptr_t)fd);
    // } else if(modify) {
    //     EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void *)(intptr_t)fd);
    // }

//    printf("%s Socket %d events read %d\n", modify ? "mod" : "add", sock, events & kReadEvent);
    kevent(efd, ev, n, NULL, 0, NULL);
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

    int bufsize = 1024 * 1024;
    socklen_t len = sizeof(bufsize);
    setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

//    printf("[VPS:0] Accept : %s\n", pClient->m_strIPAddr);
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
 
    printf("=========> ON READ CALL ONCLOSE =====> \n");
    OnClose(sock);
}

bool AsyncMediaServerSocket::OnClose(Socket sock) {
    ClientObject* pClient = m_clientList.Find(sock);
    if(pClient != NULL) {  
        pClient->m_isClosing = true;

        // close socket
        shutdown(sock, SHUT_RDWR);
        close(sock);
//        printf("[VPS:%d] Socket %d closed\n", pClient->m_nHpNo, sock);

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

        if( !pClient->m_isExitCommandReceived && nClientType != CLIENT_TYPE_MONITOR ) {
            // 비정상적으로 연결 해제된 Client 정보를 저장하되
            // m_SocketClient 변수값은 초기화한다.
            printf("========= 비정상 종료 처리 필요 ==========\n");
            pClient->m_isClosing = false;

            return true;
        }

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
    if(iLen >= 1024 * 1024) {
        printf("패킷 사이즈 너무 큼\n");
        return false;
    }

    if(pClient->m_isClosing && !force) {
        printf("[VPS:%d] 소켓 Close 처리 중\n", pClient->m_nHpNo);
        return false;
    }

    Socket sock = pClient->m_clientSock;

    int nOffset = 0;
    int dataSize = iLen;

//    for(int i = 0; i < 5; i++) {
    while(1) {
        int nSendResult = send(sock, pData + nOffset, dataSize, 0);
        if(nSendResult == SOCKET_ERROR) {
            if(errno == EWOULDBLOCK) {
                usleep(1);
                continue;
            } else {
                printf("Socket Send() Error : %d\n", errno);
                return false;
            }
        } else {
            nOffset += nSendResult;
            dataSize -= nSendResult;

            if(nOffset < iLen) {
                continue;
            }

            return true;
        }
    }

    printf("[VPS:%d] SEND DATA FAILED : %d / %d\n", pClient->m_nHpNo, nOffset, iLen);

    return false;
}

void AsyncMediaServerSocket::OnServerEvent(int efd, ServerSocket serverSock, int waitms) {
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

        } else {
            printf("[VPS:0] unknown event\n");
        }
    }
}

int AsyncMediaServerSocket::InitSocket(void* pNetMgr, int port) {
    m_pNetMgr = pNetMgr;

    int epollfd = kqueue();
    if(epollfd < 0) {
        printf("[VPS:0] epoll create failed\n");
        return 0;
    }

    m_serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if(m_serverSock < 0) {
        printf("[VPS:0] server socket failed\n");
        return 0;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int r = ::bind(m_serverSock, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if(r) {
        printf("[VPS:0] bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));
        return 0;
    } 

    r = listen(m_serverSock, 20);
    if(r) {
        printf("[VPS:0] listen failed %d %s", errno, strerror(errno));
        return 0;
    }

//    printf("[VPS:0] VPS Server listening at %d\n", port);

    char log[128] = {0,};
    sprintf(log, "VPS Server listening : %d", port);
    ((NetManager*)m_pNetMgr)->AddLog(0, log, LOG_TO_FILE); 

    SetNonBlock(m_serverSock);
    UpdateEvents(epollfd, m_serverSock, kReadEvent, false);

    while(m_isRunServer) {
        OnServerEvent(epollfd, m_serverSock, 10000);
    }

    printf("========= CLOSE SERVER SOCKET ==========\n");

    return 0;
}

ClientObject* AsyncMediaServerSocket::Find(Socket sock) {
    return m_clientList.Find(sock);
}

ClientObject* AsyncMediaServerSocket::FindHost(int nHpNo) {
    return m_clientList.FindHost(nHpNo);
}

ClientObject* AsyncMediaServerSocket::FindGuest(int nHpNo, char* id) {
    return m_clientList.FindGuest(nHpNo, id);
}

ClientObject* AsyncMediaServerSocket::GetMobileController() {
    return m_clientList.GetMobileController();
}

ClientObject* AsyncMediaServerSocket::FindMonitor(int nHpNo) {
    return m_clientList.FindMonitor(nHpNo);
}

void AsyncMediaServerSocket::UpdateClientList(ClientObject* pClient) {
    m_clientList.UpdateClient(pClient);
}

ClientObject** AsyncMediaServerSocket::GetClientList(int nHpNo) {
    return m_clientList.GetClientList(nHpNo);
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
    int nHpNo = pClient->m_nHpNo;
    m_clientList.Delete(pClient->m_clientSock);

    delete pClient;
    pClient = NULL;
    printf("============== DELETE CLIENT ============== %d\n", nHpNo);
}