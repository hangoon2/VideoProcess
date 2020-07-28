#include "MirrorCommon.h"

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