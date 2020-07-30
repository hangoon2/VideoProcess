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
    ClientObject* FindGuest(int nHpNo, char* id);

    void UpdateClient(ClientObject* pClient);

    ClientObject* GetMobileController();
    ClientObject** GetClientList(int nHpNo);

private:
    void DeleteClient(ClientObject* pClient);

private:
    map<Socket, ClientObject*> m_clientMap;

    ClientObject* m_clientList[MAXCHCNT][MAXCLIENT_PER_CH];
    ClientObject* m_mobileController;
};

#endif