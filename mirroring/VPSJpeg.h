#ifndef VPS_JPEG_H
#define VPS_JPEG_H

#include "../common/VPSCommon.h"

class VPSJpeg {
public:
    VPSJpeg();
    virtual ~VPSJpeg();

//    int RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality);
    int RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height);
//    int RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality);
    int RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height);

    bool SaveJpeg(char* filePath, BYTE* pJpgSrc, int nJpgSrcLen, int quality);
    bool Write_to_jpegfile(char* filename, BYTE* pJpgSrc, int width, int height, int quality);

    BYTE* Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen);

private:
    int Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen, BYTE* pOut);
    int Encode_Jpeg(BYTE* pJpgSrc, BYTE* pOut, int quality, int width, int height);

    BYTE* m_pJpgData;
    BYTE* m_pRgbData;
};

#endif