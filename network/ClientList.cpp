#include "ClientList.h"

ClientList::ClientList() {
    for(int i = 0; i < MAXCHCNT; i++) {
        for(int j = 0; j < MAXCLIENT_PER_CH; j++) {
            m_clientList[i][j] = NULL;
        }
    }
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
    ClientObject* pClient = Find(sock);
    if(pClient != NULL) {
        if(pClient->m_nClientType == CLIENT_TYPE_HOST 
            || pClient->m_nClientType == CLIENT_TYPE_GUEST 
            || pClient->m_nClientType == CLIENT_TYPE_MONITOR) {
            DeleteClient(pClient);
        }

        m_clientMap.erase(sock);
    }
}

ClientObject* ClientList::FindHost(int nHpNo) {
    for(auto it = m_clientMap.begin(); it != m_clientMap.end(); it++) {
        ClientObject* pClient = it->second;
        if(pClient->m_nHpNo == nHpNo && pClient->m_nClientType == CLIENT_TYPE_HOST) {
            return pClient;
        }        
    }

    return NULL;
}

ClientObject* ClientList::FindUnknown() {
    for(auto it = m_clientMap.begin(); it != m_clientMap.end(); it++) {
        ClientObject* pClient = it->second;
        if(pClient->m_nHpNo == 0 && pClient->m_nClientType == CLIENT_TYPE_UNKNOWN) {
            return pClient;
        }        
    }

    return NULL;
}

void ClientList::UpdateClient(ClientObject* pClient) {
    if(pClient->m_nClientType == CLIENT_TYPE_MC) {
        m_mobileController = pClient;
    } else {
        int nHpNo = pClient->m_nHpNo;
        for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
            if(m_clientList[nHpNo - 1][i] == NULL) {
                m_clientList[nHpNo - 1][i] = pClient;
                printf("UPDATE CLIENT LIST\n");
                break;
            }
        }
    }
}

void ClientList::DeleteClient(ClientObject* pClinet) {
    int nHpNo = pClinet->m_nHpNo;
    for(int i = 0; i < MAXCLIENT_PER_CH; i++) {
        if(m_clientList[nHpNo - 1][i] == pClinet) {
            printf("DELETE CLIENT LIST\n");
            m_clientList[nHpNo - 1][i] = NULL;
            break;
        }
    }
}

ClientObject* ClientList::GetMobileController() {
    return m_mobileController;
}

ClientObject** ClientList::GetClientList(int nHpNo) {
    return m_clientList[nHpNo - 1];
}