#ifndef VPS_JPEG_H
#define VPS_JPEG_H

#include "../common/VPSCommon.h"

class VPSJpeg {
public:
    VPSJpeg();
    virtual ~VPSJpeg();

    int RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality);
    int RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality);

    bool SaveJpeg(char* filePath, BYTE* pJpgSrc, int nJpgSrcLen, int quality);
};

#endif