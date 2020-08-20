#include "network/NetManager.h"

#include <stdio.h>
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

void* BackgroundThreadFunc(void* pArg) {
    pNetMgr = new NetManager(NULL);
    pNetMgr->OnServerModeStart();

    return NULL;
}

void threadFunc() {
    pNetMgr = new NetManager(NULL);
    pNetMgr->OnServerModeStart();
}

int main(int argc, char *argv[]) {
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
        memset( &pCap[i], 0x00, sizeof(HDCAP) );
    }
#endif

#if ENABLE_UI
//    Glib::thread_init();

    if(!Glib::thread_supported()) {
        Glib::thread_init();
    }

    pthread_create(&gs_tID, NULL, &BackgroundThreadFunc, NULL);

    auto app = Gtk::Application::create(argc, argv, "com.onycom.vps");

    // g_type_init();

    // if(!g_thread_supported()) {
    //     g_thread_init(NULL);
    // }

    // gdk_threads_init();
    // gdk_threads_enter();

    vps = new VPS();

//    Glib::Thread::create(sigc::ptr_fun(&threadFunc), true);

    int ret = app->run(*vps);
#else
    pNetMgr = new NetManager(NULL);
    pNetMgr->OnServerModeStart();
#endif

    ExitProgram();
    printf("EXIT PROGRAM\n");

    return 0;
}