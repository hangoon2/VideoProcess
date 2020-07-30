#ifndef ASYNC_MEDIA_SERVER_SOCKET_H
#define ASYNC_MEDIA_SERVER_SOCKET_H

#include "../common/VPSCommon.h"
#include "ClientObject.h"
#include "ClientList.h"

typedef bool (*PDATA_READ_ROUTINE)(ClientObject* pClient, char* pRcvData, int len);

class AsyncMediaServerSocket {
public:
    AsyncMediaServerSocket();
    ~AsyncMediaServerSocket();

    int InitSocket(int port, PDATA_READ_ROUTINE pOnReadEx);
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
    PDATA_READ_ROUTINE m_pOnReadEx;

    ClientList m_clientList;
};

#endif