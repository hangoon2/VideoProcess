#include <stdio.h>

#include "network/NetManager.h"

int main() {
    NetManager *pNetMgr = NULL;

    pNetMgr = new NetManager();
    pNetMgr->OnServerModeStart();

    if(pNetMgr != NULL) {
        delete pNetMgr;
        pNetMgr = NULL;
    }

    return 0;
}