#include "VPSCommon.h"

#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

UINT16 CalChecksum(UINT16* ptr, int nbytes) {
    INT64 sum;
    UINT16 answer;

    sum =0;
    while(nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if(nbytes == 1) {
        sum += *(BYTE*)ptr;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

UINT16 SwapEndianU2(UINT16 wValue) {
    return (UINT16)((wValue >> 8) | (wValue << 8));
}

uint32_t SwapEndianU4(UINT nValue) {
    char bTmp;
    bTmp = *((char*)&nValue + 3);
    *((char*)&nValue + 3) = *((char*)&nValue + 0);
    *((char*)&nValue + 0) = bTmp;

    bTmp = *((char*)&nValue + 2);
    *((char*)&nValue + 2) = *((char*)&nValue + 1);
    *((char*)&nValue + 1) = bTmp;

    return nValue;
}

BYTE* MakeSendData2(short usCmd, int nHpNo, int dataLen, BYTE* pData, BYTE* pDstData, int& totLen) {
    int dataSum = 0;
    int sizeofData = 0;

    memset(pDstData, 0x00, SEND_BUF_SIZE);

    BYTE mStartFlag = CMD_START_CODE;
    sizeofData = sizeof(mStartFlag);
    memcpy(pDstData, (char*)&mStartFlag, sizeofData);
    dataSum = sizeofData;

    INT mDataSize = htonl(dataLen);
    sizeofData = sizeof(mDataSize);
    memcpy(pDstData + dataSum, (char*)&mDataSize, sizeofData);
    dataSum += sizeofData;

    UINT16 mCommandCode = htons(usCmd);
    sizeofData = sizeof(mCommandCode);
    memcpy(pDstData + dataSum, (char*)&mCommandCode, sizeofData);
    dataSum += sizeofData;

    BYTE mDeviceNo = (BYTE)nHpNo;
    sizeofData = sizeof(mDeviceNo);
    memcpy(pDstData + dataSum, (char*)&mDeviceNo, sizeofData);
    dataSum += sizeofData;

    if(pData != NULL) {
        sizeofData = dataLen;
        memcpy(pDstData + dataSum, pData, dataLen);
        dataSum += sizeofData;
    }

    UINT16 mChecksum = htons( CalChecksum((UINT16*)(pDstData + 1), ntohl(mDataSize) + CMD_HEAD_SIZE - 1) );
    sizeofData = sizeof(mChecksum);
    memcpy(pDstData + dataSum, (char*)&mChecksum, sizeofData);
    dataSum += sizeofData;

    BYTE mEndFlag = CMD_END_CODE;
    sizeofData = sizeof(mEndFlag);
    memcpy(pDstData + dataSum, (char*)&mEndFlag, sizeofData);
    dataSum += sizeofData;

    totLen = dataSum;

    return pDstData;
}

void GetLocalTime(SYSTEM_TIME &stTime) {
    struct timeval time;

    gettimeofday(&time, 0);

    struct tm* currentTime = localtime(&time.tv_sec);

    stTime.year = currentTime->tm_year + 1900;
    stTime.month = currentTime->tm_mon + 1;
    stTime.day = currentTime->tm_mday;
    stTime.hour = currentTime->tm_hour;
    stTime.minute = currentTime->tm_min;
    stTime.second = currentTime->tm_sec;
    stTime.millisecond = time.tv_usec / 1000;
}

int GetCurrentDay() {
    SYSTEM_TIME stTime;
    GetLocalTime(stTime);

    return stTime.day;
}

ULONGLONG GetTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000ull) + (ts.tv_nsec/1000ull/1000ull);

    // timeval tv;
    // gettimeofday(&tv, NULL);
    // return (ULONG)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

bool DoesFileExist(const char* filePath) {
    return access(filePath, 0) == 0;
}

void GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char* value) {
    ifstream file;
    file.open("Setup.ini");
    if( file.is_open() ) {
        char line[256] = {0,};
        bool findSection = false;
        bool findKey = false;

        while( file.getline(line, sizeof(line) ) ) {
//            int ret = strncmp( line, section, strlen(section) );
            if( strncmp( line, section, strlen(section) ) == 0 ) {
                findSection = true;
            }

            if( findSection && strncmp( line, key, strlen(key) ) == 0 ) {
                strcpy(value, &line[strlen(key) + 1]);
                value[strlen(value) - 1] = '\0';
                findKey = true;
                break;
            }
        }

        if(!findKey) {
            strcpy(value, defaultValue);
        }
    }

    file.close();
}

void* Shared_GetPointer() {
    int sharedId = shmget((key_t)SAHRED_MEMORY_KEY, sizeof(HDCAP) * MEM_SHARED_MAX_COUNT, 0666);
    if(sharedId < 0) {
        printf("Shared Memory : Failed Get Pointer\n");
        return NULL;
    }

    return shmat(sharedId, NULL, 0); 
}