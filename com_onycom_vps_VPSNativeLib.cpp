#include "com_onycom_vps_VPSNativeLib.h"
#include "network/NetManager.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>

static NetManager *gs_pNetMgr = NULL;
static pthread_t gs_tID = NULL;

static int shmid = -1;
static void* shared_memory = (void *)0;

static bool gs_isRunnging = true;
static bool gs_updateLog = false;
static char gs_log[512] = {0,};

void NativeLogCallback(int nHpNo, const char* log, vps_log_target_t nTarget) {
    if(!gs_isRunnging) return;

    if(nTarget == LOG_TO_UI || nTarget == LOG_TO_BOTH) {
        gs_updateLog = false;

        memset( gs_log, 0x00, sizeof(gs_log) );
        strcpy(gs_log, log);

        gs_updateLog = true;
    }
}

void* NetworkManagerStartFunc(void* pArg) {
    gs_pNetMgr = new NetManager(NativeLogCallback);
    gs_pNetMgr->OnServerModeStart();

    return NULL;
}

/*
 * Class:     com_onycom_vps_VPSNativeLib
 * Method:    Start
 * Signature: 
 */
JNIEXPORT void JNICALL Java_com_onycom_vps_VPSNativeLib_Start
(JNIEnv *env, jobject object) {
    printf("VPS NATIVE START\n");

    HDCAP* pCap= NULL;
    
	// 공유메모리 공간을 만든다.
	shmid = shmget((key_t)SAHRED_MEMORY_KEY, sizeof(HDCAP) * MEM_SHARED_MAX_COUNT, 0666 | IPC_CREAT);
	if(shmid == -1) {
		perror("shmget failed : ");
		return;
	}

	// 공유메모리를 사용하기 위해 프로세스메모리에 붙인다. 
	shared_memory = shmat(shmid, (void*)0, 0);
	if (shared_memory == (void*)-1) {
		perror("shmat failed : ");
		return;
	}

	pCap = (HDCAP*)shared_memory;

    for(int i = 0 ;i < MEM_SHARED_MAX_COUNT; i++){
        memset( &pCap[i], 0x00, sizeof(HDCAP) );
    }

    pthread_create(&gs_tID, NULL, &NetworkManagerStartFunc, NULL);

    jclass cls = env->GetObjectClass(object);
    jmethodID mid1 = env->GetMethodID(cls, "CallbackLog", "(ILjava/lang/String;)V");
    if(mid1 == NULL) {
        printf("==================> NOT FOUND CALLBACK METHOD\n");
        return;
    }

    while(gs_isRunnging) {
        if(gs_updateLog) {
            jstring strLog = env->NewStringUTF(gs_log);
            env->CallObjectMethod(object, mid1, 1, strLog);
            gs_updateLog = false;
        }

        usleep(100);
    }

    printf("VPS NATIVE START FUNCTION END\n");
}

/*
 * Class:     com_onycom_vps_VPSNativeLib
 * Method:    Stop
 * Signature: 
 */
JNIEXPORT void JNICALL Java_com_onycom_vps_VPSNativeLib_Stop
(JNIEnv *env, jobject object) {
    printf("VPS NATIVE STOP\n");
    gs_isRunnging = false;

    if(gs_pNetMgr != NULL) {
        gs_pNetMgr->OnDestroy();

        delete gs_pNetMgr;
        gs_pNetMgr = NULL;
    }

    if(shared_memory != NULL) {
        shmdt(shared_memory);
    }

    if(shmid != -1) {
        shmctl(shmid, IPC_RMID, 0);
    }
}

/*
 * Class:     com_onycom_vps_VPSNativeLib
 * Method:    GetLastScene
 * Signature: 
 */
JNIEXPORT jint JNICALL Java_com_onycom_vps_VPSNativeLib_GetLastScene
  (JNIEnv *env, jobject object, jint nHpNo, jbyteArray pDstJpg) {
    if(gs_pNetMgr != NULL) {
        if( gs_pNetMgr->IsOnService(nHpNo) ) {
            BYTE* pDst = (BYTE*)malloc(MIR_DEFAULT_MEM_POOL_UINT);

            int ret = gs_pNetMgr->GetLastScene(nHpNo, pDst);
            if(ret > 0) {
                env->SetByteArrayRegion(pDstJpg, 0, ret, (jbyte*)reinterpret_cast<BYTE*>(pDst));
            }

            free(pDst);

            return ret;
        }
    }

    return 0;
}