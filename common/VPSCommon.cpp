#include "VPSCommon.h"

#include <string.h>
#include <arpa/inet.h>

ONYPACKET_UINT16 CalChecksum(unsigned short* ptr, int nbytes) {
    ONYPACKET_INT32 sum;
    ONYPACKET_UINT16 answer;

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

uint16_t SwapEndianU2(uint16_t wValue) {
    return (uint16_t)((wValue >> 8) | (wValue << 8));
}

uint32_t SwapEndianU4(uint32_t nValue) {
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

    ONYPACKET_INT mDataSize = htonl(dataLen);
    sizeofData = sizeof(mDataSize);
    memcpy(pDstData + dataSum, (char*)&mDataSize, sizeofData);
    dataSum += sizeofData;

    ONYPACKET_UINT16 mCommandCode = htons(usCmd);
    sizeofData = sizeof(mCommandCode);
    memcpy(pDstData + dataSum, (char*)&mCommandCode, sizeofData);
    dataSum += sizeofData;

    BYTE mDeviceNo = (BYTE)nHpNo;
    sizeofData = sizeof(mDeviceNo);
    memcpy(pDstData + dataSum, (char*)&mDeviceNo, sizeofData);
    dataSum += sizeofData;

    if(dataLen > 0) {
        sizeofData = dataLen;
        memcpy(pDstData + dataSum, pData, dataLen);
        dataSum += sizeofData;
    }

    ONYPACKET_UINT16 mChecksum = htons( CalChecksum((ONYPACKET_UINT16*)(pDstData + 1), ntohl(mDataSize) + CMD_HEAD_SIZE - 1) );
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