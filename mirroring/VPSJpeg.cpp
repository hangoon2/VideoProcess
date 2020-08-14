#include "VPSJpeg.h"

#include <iostream>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#include <stdio.h>

int Encode(BYTE* pJpgSrc, Mat& img, int quality) {
    vector<uchar> buff;
    vector<int> param = vector<int>(2);
    param[0] = IMWRITE_JPEG_QUALITY;
    param[1] = quality;

    imencode(".jpg", img, buff, param);

    memcpy( pJpgSrc, (const void*)reinterpret_cast<uchar*>(&buff[0]), buff.size() );

    return buff.size();
}

VPSJpeg::VPSJpeg() {
    
}

VPSJpeg::~VPSJpeg() {

}

int VPSJpeg::RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
    Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);

//    rotate(rawImage, rawImage, ROTATE_90_COUNTERCLOCKWISE);
    transpose(rawImage, rawImage);
    flip(rawImage, rawImage, 0);

    return Encode(pJpgSrc, rawImage, quality);
}

int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
    Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);
    
    transpose(rawImage, rawImage);
    flip(rawImage, rawImage, 1);
    
    return Encode(pJpgSrc, rawImage, quality);
}

bool VPSJpeg::SaveJpeg(char* filePath, BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
    Mat rawData = Mat(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc).clone();
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);

    vector<uchar> buff;
    vector<int> param = vector<int>(2);
    param[0] = IMWRITE_JPEG_QUALITY;
    param[1] = quality;

    imencode(".jpg", rawImage, buff, param);

    imwrite(filePath, rawImage);

    rawData.release();
    rawImage.release();

    return true;
}