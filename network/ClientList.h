#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include "../common/VPSCommon.h"
#include "ClientObject.h"

#include <map>

using namespace std;

class ClientList {
public:
    ClientList();
    virtual ~ClientList();

    void Clear();
    void Insert(Socket sock, ClientObject* pClient);
    ClientObject* Find(Socket sock);
    void Delete(Socket sock);

    ClientObject* FindHost(int nHpNo);
    ClientObject* FindUnknown();

    void InsertClient(ClientObject* pClient);
    
    ClientObject** GetClientList(int nHpNo);

private:
    void DeleteClient(ClientObject* pClient);

private:
    map<Socket, ClientObject*> m_clientMap;

    ClientObject* m_clientList[MAXCHCNT][MAXCLIENT_PER_CH];
};

#endif