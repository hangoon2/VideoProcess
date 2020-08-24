#include "network/NetManager.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

static int shmid = -1;
static void* shared_memory = (void *)0;

#if ENABLE_UI
#include "VPS.h"

static VPS* vps = NULL;
static pthread_t gs_tID = NULL;
#endif

static NetManager *pNetMgr = NULL;

void ExitProgram() {
#if ENABLE_UI
    if(vps != NULL) {
        delete vps;
        vps = NULL;
    }
#endif

    if(pNetMgr != NULL) {
        pNetMgr->OnDestroy();

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

    printf("VPS 종료\n");
}

void* BackgroundThreadFunc(void* pArg) {
    pNetMgr = new NetManager(NULL);
    pNetMgr->OnServerModeStart();

    return NULL;
}

int main(int argc, char *argv[]) {
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
        memset( &pCap[i], 0x00, sizeof(HDCAP) );
    }
#endif

#if ENABLE_UI
    pthread_create(&gs_tID, NULL, &BackgroundThreadFunc, NULL);
    
    vps = new VPS();
    vps->ShowWindow();
#else
    pNetMgr = new NetManager(NULL);
    pNetMgr->OnServerModeStart();
#endif

    ExitProgram();

    return 0;
}