#include "com_onycom_vps_VPSNativeLib.h"
#include "../VPS.h"

#include <stdio.h>
#include <unistd.h>

static VPS* gs_pVps = NULL;

static bool gs_isRunnging = true;

#if ENABLE_JAVA_UI

/*
 * Class:     com_onycom_vps_VPSNativeLib
 * Method:    Start
 * Signature: 
 */
JNIEXPORT void JNICALL Java_com_onycom_vps_VPSNativeLib_Start
(JNIEnv *env, jobject object) {
    printf("VPS NATIVE START\n");
    gs_pVps = new VPS();
    gs_pVps->Start();

    jclass cls = env->GetObjectClass(object);

    jmethodID callbackFunc = env->GetMethodID(cls, "CallbackFunc", "(IILjava/lang/String;)V");
    if(callbackFunc == NULL) {
        printf("==================> NOT FOUND CALLBACK METHOD\n");
        return;
    }

    CALLBACK cb;
    while(gs_isRunnging) {
        if( gs_pVps->GetLastCallback(&cb) ) {
            jstring strData = env->NewStringUTF(cb.data);
            env->CallObjectMethod(object, callbackFunc, cb.nHpNo, cb.type, strData);
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

    if(gs_pVps != NULL) {
        gs_pVps->Stop();

        delete gs_pVps;
        gs_pVps = NULL;
    }
}

/*
 * Class:     com_onycom_vps_VPSNativeLib
 * Method:    GetLastScene
 * Signature: 
 */
JNIEXPORT jint JNICALL Java_com_onycom_vps_VPSNativeLib_GetLastScene
  (JNIEnv *env, jobject object, jint nHpNo, jbyteArray pDstJpg) {
    if(gs_pVps != NULL) {
        if( gs_pVps->IsOnService(nHpNo) ) {
            BYTE* pDst = (BYTE*)malloc(MAX_SIZE_RGB_DATA);

            int ret = gs_pVps->GetLastScene(nHpNo, pDst);
            if(ret > 0) {
                env->SetByteArrayRegion(pDstJpg, 0, ret, (jbyte*)reinterpret_cast<BYTE*>(pDst));
            }

            free(pDst);

            return ret;
        }
    }

    return 0;
}

#endif