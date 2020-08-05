#include <stdio.h>

#include "network/NetManager.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int shmid = -1;
static void* shared_memory = (void *)0;
static NetManager *pNetMgr = NULL;

void ExitProgram() {
    if(pNetMgr != NULL) {
        delete pNetMgr;
        pNetMgr = NULL;
    }

#if ENABLE_SHARED_MEMORY
    if(shared_memory != NULL) {
        shmdt(shared_memory);
    }

    if(shmid != -1) {
        shmctl(shmid, IPC_RMID, 0);
    }
#endif
}

void SigHandler(int sig) {
    if(sig == SIGINT) {
        ExitProgram();
        exit(0);
    }
}

int main() {
    signal(SIGINT, SigHandler);

#if ENABLE_SHARED_MEMORY
	HDCAP* pCap= NULL;
    
	// 공유메모리 공간을 만든다.
	shmid = shmget((key_t)SAHRED_MEMORY_KEY, sizeof(HDCAP) * MEM_SHARED_MAX_COUNT, 0666 | IPC_CREAT);
	if(shmid == -1) {
		perror("shmget failed : ");
		exit(0);
	}

	// 공유메모리를 사용하기 위해 프로세스메모리에 붙인다. 
	shared_memory = shmat(shmid, (void*)0, 0);
	if (shared_memory == (void*)-1) {
		perror("shmat failed : ");
		exit(0);
	}

	pCap = (HDCAP*)shared_memory;

    for(int i = 0 ;i < MEM_SHARED_MAX_COUNT; i++){
        memset(&pCap[i], 0x00, sizeof(HDCAP));
    }
#endif

    pNetMgr = new NetManager();
    pNetMgr->OnServerModeStart();

    ExitProgram();

    return 0;
}