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
    bool OnClose(Socket sock);
    bool OnSend(Socket sock, BYTE* pData, int iLen, bool force);

    ClientObject* FindHost(int nHpNo);
    ClientObject* FindGuest(int nHpNo, char* id);

    ClientObject* GetMobileController();
    void UpdateClientList(ClientObject* pClient);
    ClientObject** GetClientList(int nHpNo);
    
private:
    void SetNonBlock(Socket sock);
    void UpdateEvents(int efd, Socket sock, int events, bool modify);
    void OnAccept(int efd, ServerSocket serverSock);
    void OnRead(int efd, Socket sock);
    void OnServerEvent(int efd, ServerSocket serverSock, int waitms);

private:
    ServerSocket m_serverSock;
    ClientList m_clientList;

    void* m_pNetMgr;
};

#endif