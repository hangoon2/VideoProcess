#include "ClientList.h"

ClientList::ClientList() {

}

ClientList::~ClientList() {

}

void ClientList::Clear() {
    for(auto it = m_clientMap.begin(); it != m_clientMap.end(); it++) {
        ClientObject* pObject = it->second;
        
        delete pObject;
        pObject = NULL;
    }
}

void ClientList::Insert(Socket sock, ClientObject* pObject) {
    m_clientMap.insert(pair<Socket, ClientObject*>(sock, pObject));
}

ClientObject* ClientList::Find(Socket sock) {
    return m_clientMap.find(sock)->second;
}