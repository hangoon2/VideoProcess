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

#define exit_if(r, ...) if(r) {printf(__VA_ARGS__); printf("\nerror no: %d error msg %s\n", errno, strerror(errno)); exit(1);}

const int kReadEvent = 1;
const int kWriteEvent = 2;

AsyncMediaServerSocket::AsyncMediaServerSocket() {
    m_serverSock = INVALID_SOCKET;

    m_pNetMgr = NULL;
}

AsyncMediaServerSocket::~AsyncMediaServerSocket() {
    m_clientList.Clear();

    if(m_serverSock != INVALID_SOCKET) {
        printf("[VPS:0] Closed Listening Server\n");
        close(m_serverSock);
        m_serverSock = INVALID_SOCKET;
    }
}

void AsyncMediaServerSocket::SetNonBlock(Socket sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");

    int r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    exit_if(r < 0, "fcntl failed");
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
    int r = kevent(efd, ev, n, NULL, 0, NULL);
    exit_if(r, "kevent failed");
}

void AsyncMediaServerSocket::OnAccept(int efd, ServerSocket serverSock) {
    struct sockaddr_in raddr;
    memset( &raddr, 0x00, sizeof(raddr) );
    socklen_t rsz = sizeof(raddr);
    Socket clientSock = accept(serverSock, (struct sockaddr *)&raddr, &rsz);
    exit_if(clientSock < 0, "accept failed");

    struct sockaddr_in peer;
    memset( &peer, 0x00, sizeof(peer) );
    socklen_t alen = sizeof(peer);
    int r = getpeername(clientSock, (sockaddr *)&peer,  &alen);
    exit_if(r < 0, "getpeername failed");

    SetNonBlock(clientSock);
    UpdateEvents(efd, clientSock, kReadEvent, false);

    ClientObject* pClient = new ClientObject();
    pClient->m_clientSock = clientSock;
    strcpy( pClient->m_strIPAddr, inet_ntoa(raddr.sin_addr) );

    m_clientList.Insert(clientSock, pClient);

    printf("[VPS:0] Accept : %s\n", pClient->m_strIPAddr);

    //write( clientSock, "connect server", sizeof("connect server") );
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
    // close socket
    shutdown(sock, SHUT_RDWR);
    close(sock);

    ClientObject* pClient = m_clientList.Find(sock);
    if(pClient != NULL) {  
        printf("[VPS:%d] Socket %d closed\n", pClient->m_nHpNo, sock);

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
            m_clientList.Delete(sock);
            
            delete pClient;
            pClient = NULL;
            
            return true;
        }

        if( !pClient->m_isExitCommandReceived && nClientType != CLIENT_TYPE_MONITOR ) {
            // 비정상적으로 연결 해제된 Client 정보를 저장하되
            // m_SocketClient 변수값은 초기화한다.
            printf("========= 비정상 종료 처리 필요 ==========\n");
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

            usleep(100);
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

bool AsyncMediaServerSocket::OnSend(Socket sock, BYTE* pData, int iLen, bool force) {
    int ret = (int)write(sock, pData, iLen);
    if(ret != iLen) {
        return false;
    }

    return true;
}

void AsyncMediaServerSocket::OnServerEvent(int efd, ServerSocket serverSock, int waitms) {
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;
    const int kMaxEvents = 20;
    struct kevent activeEvs[kMaxEvents];
    int n = kevent(efd, NULL, 0, activeEvs, kMaxEvents, &timeout);
//    printf("epoll_wait return %d\n", n);

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
            exit_if(1, "unknown event");
        }
    }
}

int AsyncMediaServerSocket::InitSocket(void* pNetMgr, int port) {
    m_pNetMgr = pNetMgr;

    int epollfd = kqueue();
    exit_if(epollfd < 0, "epoll_create failed");

    m_serverSock = socket(AF_INET, SOCK_STREAM, 0);
    exit_if(m_serverSock < 0, "socket failed");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int r = ::bind(m_serverSock, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    exit_if(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));

    r = listen(m_serverSock, 20);
    exit_if(r, "listen failed %d %s", errno, strerror(errno));

    printf("[VPS:0] VPS Server listening at %d\n", port);

    SetNonBlock(m_serverSock);
    UpdateEvents(epollfd, m_serverSock, kReadEvent, false);

    for(;;) {
        OnServerEvent(epollfd, m_serverSock, 10000);
    }

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
    printf("============== DELETE CLIENT ==============\n");
    m_clientList.Delete(pClient->m_clientSock);

    printf("============== DELETE CLIENT IN ==============\n");
    delete pClient;
    pClient = NULL;
    printf("============== DELETE CLIENT OUT ==============\n");
}