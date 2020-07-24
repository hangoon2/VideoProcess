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
    return m_clientMap.find(sock)->second;
}