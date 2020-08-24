#include "VPSJpeg.h"

#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

extern "C" {
#include <jpeglib.h>
//#include <turbojpeg.h>
}

#include <setjmp.h>

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
    m_pJpgData = (BYTE*)malloc(1024 * 1024);
    m_pRgbData = (BYTE*)malloc(960 * 960 * 3);
}

VPSJpeg::~VPSJpeg() {
    free(m_pJpgData);
    free(m_pRgbData);
}

// int VPSJpeg::RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
//     ULONGLONG start = GetTickCount();
//     Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
//     ULONGLONG ret1 = GetTickCount();
//     Mat rawImage = imdecode(rawData, IMREAD_COLOR);
//     ULONGLONG ret2 = GetTickCount();

//     rotate(rawImage, rawImage, ROTATE_90_COUNTERCLOCKWISE);
//     // transpose(rawImage, rawImage);
//     // flip(rawImage, rawImage, 0);
//     ULONGLONG ret3 = GetTickCount();
//     return 0;

//     // int r = Encode(pJpgSrc, rawImage, quality);
//     // ULONGLONG ret4 = GetTickCount();

//     // printf("SCREEN ROTATE TIME : %lld, %lld, %lld, %lld\n", (ret1 - start), (ret2 - ret1), (ret3 - ret2), (ret4 - ret3));

//     // return r;
// }

int VPSJpeg::RotateLeft(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height) {
    Decode_Jpeg(pJpgSrc, nJpgSrcLen, m_pRgbData);
    Mat rgbImage(height, width, CV_8UC3, m_pRgbData);

    rotate(rgbImage, rgbImage, ROTATE_90_COUNTERCLOCKWISE);
    // transpose(rawImage, rawImage);
    // flip(rawImage, rawImage, 0);

    return Encode_Jpeg(rgbImage.data, pJpgSrc, quality, height, width);
}

// int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
//     Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
//     Mat rawImage = imdecode(rawData, IMREAD_COLOR);
    
//     transpose(rawImage, rawImage);
//     flip(rawImage, rawImage, 1);
    
//     return Encode(pJpgSrc, rawImage, quality);
// }

int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height) {
    Decode_Jpeg(pJpgSrc, nJpgSrcLen, m_pRgbData);
    Mat rgbImage(height, width, CV_8UC3, m_pRgbData);

    rotate(rgbImage, rgbImage, ROTATE_90_CLOCKWISE);
    // transpose(rawImage, rawImage);
    // flip(rawImage, rawImage, 1);
    
    return Encode_Jpeg(rgbImage.data, pJpgSrc, quality, height, width);
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

int VPSJpeg::Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen, BYTE* pOut) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, pJpgSrc, nJpgSrcLen);

    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int pixel_size = cinfo.output_components;

    unsigned long buf_size = width * height * pixel_size;
//    printf("BUFFER SIZE : %d, %d, %d, %ld\n", width, height, pixel_size, buf_size);

    int row_stride = width * pixel_size;

    while(cinfo.output_scanline < cinfo.output_height) {
        unsigned char* buffer_array[1];
        buffer_array[0] =  pOut + (cinfo.output_scanline) * row_stride;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return (int)buf_size;
}

int VPSJpeg::Encode_Jpeg(BYTE* pJpgSrc, BYTE* pOut, int quality, int width, int height) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned long outLen = 0; 

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    jpeg_mem_dest(&cinfo, (unsigned char**)&m_pJpgData, &outLen);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_EXT_RGB;
//    cinfo.dct_method = JDCT_IFAST;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPLE* image_buffer = (JSAMPLE*)(pJpgSrc);
    JSAMPROW row_pointer[1];
    int row_stride = width * cinfo.input_components;

    while(cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);

    memcpy(pOut, (const void*)m_pJpgData, outLen);

    return (int)outLen;
}

bool VPSJpeg::Write_to_jpegfile(char* filename, BYTE* pJpgSrc, int width, int height, int quality) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* outfile;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    if( (outfile = fopen(filename, "wb")) == NULL ) {
        return false;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_EXT_RGB;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = width * cinfo.input_components;

    JSAMPLE* image_buffer = (JSAMPLE*)pJpgSrc;
    JSAMPROW row_pointer[1];

    while(cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[cinfo.next_scanline  * row_stride];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(outfile);

    jpeg_destroy_compress(&cinfo);

    return true;
}

BYTE* VPSJpeg::Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen) {
    Decode_Jpeg(pJpgSrc, nJpgSrcLen, m_pRgbData); 
    return m_pRgbData;
}