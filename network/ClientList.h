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

private:
    map<Socket, ClientObject*> m_clientMap;
};

#endif