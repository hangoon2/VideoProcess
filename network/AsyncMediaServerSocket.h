#ifndef ASYNC_MEDIA_SERVER_SOCKET_H
#define ASYNC_MEDIA_SERVER_SOCKET_H

#include "../common/VPSCommon.h"
#include "ClientObject.h"
#include "ClientList.h"

class AsyncMediaServerSocket {
public:
    AsyncMediaServerSocket();
    ~AsyncMediaServerSocket();

    int InitSocket(void* pNetMgr, int port);
    void DestSocket();
    bool OnClose(Socket sock);
    bool OnSend(ClientObject* pClient, BYTE* pData, int iLen, bool force);

    ClientObject* Find(Socket sock);
    ClientObject* FindHost(int nHpNo);
    ClientObject* FindGuest(int nHpNo, char* id);
    ClientObject* FindMonitor(int nHpNo);

    ClientObject* GetMobileController();
    void UpdateClientList(ClientObject* pClient);
    ClientObject** GetClientList(int nHpNo);
    
private:
    void SetBlock(Socket sock);
    void SetNonBlock(Socket sock);
    void UpdateEvents(int efd, Socket sock, int events, bool modify);
    void OnAccept(int efd, ServerSocket serverSock);
    void OnRead(int efd, Socket sock);
    bool OnServerEvent(int efd, ServerSocket serverSock, int waitms);
    void CloseAllGuest(int nHpNo);
    void DeleteClient(ClientObject* pClient);

private:
    ServerSocket m_serverSock;
    ClientList m_clientList;

    void* m_pNetMgr;

    bool m_isRunServer;
    bool m_isClosed;

    int m_queueID;
};

#endif