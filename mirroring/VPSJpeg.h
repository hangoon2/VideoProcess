#ifndef VPS_JPEG_H
#define VPS_JPEG_H

#include "../common/VPSCommon.h"

class VPSJpeg {
public:
    VPSJpeg();
    virtual ~VPSJpeg();

    int RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height);
    int RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height);

    bool EncodeJpeg(char* filePath, BYTE* pJpgSrc, int nJpgSrcLen, int width, int height, int quality);
    BYTE* Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen);

private:
    int Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen, BYTE* pOut);
    int Encode_Jpeg(BYTE* pJpgSrc, BYTE* pOut, int quality, int width, int height);
    bool Write_to_jpegfile(char* filename, BYTE* pRgbSrc, int width, int height, int quality);

    BYTE* m_pJpgData;
    BYTE* m_pRgbData;
};

#endif