#include "MIR_Client.h"

#include <unistd.h>
#include <thread>

using namespace std;

void ThreadFunc(void* pArg) {
    for(int i = 0; i < 5; i++) {
        printf("ThreadFunc ..... %d\n", i);
        sleep(1);
    }
}

MIR_Client::MIR_Client() {

}

MIR_Client::~MIR_Client() {

}

bool MIR_Client::StartRunClientThread(int nHpNo, int nMirroringPort, int nControlPort) {
    printf("StartRunClientThread .... 1\n");
    std::thread mirrThread(ThreadFunc, this);
    printf("StartRunClientThread .... 2\n");
    
    return false;
}