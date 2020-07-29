#include "VPSCommon.h"

ONYPACKET_UINT16 calChecksum(unsigned short* ptr, int nbytes) {
    ONYPACKET_INT32 sum;
    ONYPACKET_UINT16 answer;

    sum =0;
    while(nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if(nbytes == 1) {
        sum += *(ONYPACKET_UINT8*)ptr;
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