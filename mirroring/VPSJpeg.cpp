#include "VPSJpeg.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>

using namespace std;
using namespace cv;

#include <stdio.h>

VPSJpeg::VPSJpeg() {

}

VPSJpeg::~VPSJpeg() {

}

int VPSJpeg::RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
    Mat rawData = Mat(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc).clone();
    Mat rawImage = imdecode(rawData, 1);

    transpose(rawImage, rawImage);
    flip(rawImage, rawImage, 0);

    vector<uchar> buff;
    vector<int> param = vector<int>(2);
    param[0] = IMWRITE_JPEG_QUALITY;
    param[1] = quality;

    imencode(".jpg", rawImage, buff, param);

    memcpy( pJpgSrc, (const void*)reinterpret_cast<uchar*>(&buff[0]), buff.size() );

    return buff.size();
}

int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
    Mat rawData = Mat(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc).clone();
    Mat rawImage = imdecode(rawData, 1);

    transpose(rawImage, rawImage);
    flip(rawImage, rawImage, 1);

    vector<uchar> buff;
    vector<int> param = vector<int>(2);
    param[0] = IMWRITE_JPEG_QUALITY;
    param[1] = quality;

    imencode(".jpg", rawImage, buff, param);

    memcpy( pJpgSrc, (const void*)reinterpret_cast<uchar*>(&buff[0]), buff.size() );

    return buff.size();
}