#include "ClientList.h"

ClientList::ClientList() {

}

ClientList::~ClientList() {

}

void ClientList::Clear() {
    for(auto it = m_clientMap.begin(); it != m_clientMap.end(); it++) {
        ClientObject* pClient = it->second;
        
        delete pClient;
        pClient = NULL;
    }
}

void ClientList::Insert(Socket sock, ClientObject* pClient) {
    m_clientMap.insert(pair<Socket, ClientObject*>(sock, pClient));
}

ClientObject* ClientList::Find(Socket sock) {
    auto it = m_clientMap.find(sock);
    if(it == m_clientMap.end()) {
        return NULL;
    }

    return it->second;
}

void ClientList::Delete(Socket sock) {
    m_clientMap.erase(sock);
}