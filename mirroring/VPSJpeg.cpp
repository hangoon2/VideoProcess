#include "VPSJpeg.h"

#include <stdio.h>
#include <iostream>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

extern "C" {
#include <jpeglib.h>
//#include <turbojpeg.h>
}

#include <setjmp.h>

void Write_to_jpegfile(char* filename, unsigned char* memory, int width, int height) {
    JSAMPLE* image_buffer = (JSAMPLE*)memory;
    int image_height = height;
    int image_width = width;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* outfile;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    if( (outfile = fopen(filename, "wb")) == NULL ) {
        return;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = image_width;
    cinfo.image_height = image_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_EXT_BGR;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, 90, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = width * cinfo.input_components;

    while(cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_buffer[cinfo.next_scanline  * row_stride];
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(outfile);

    jpeg_destroy_compress(&cinfo);
}

// int Encode(BYTE* pJpgSrc, Mat& img, int quality) {
//     vector<uchar> buff;
//     vector<int> param = vector<int>(2);
//     param[0] = IMWRITE_JPEG_QUALITY;
//     param[1] = quality;

//     imencode(".jpg", img, buff, param);

//     memcpy( pJpgSrc, (const void*)reinterpret_cast<uchar*>(&buff[0]), buff.size() );

//     return buff.size();
// }

const static JOCTET EOI_BUFFER[1] = {JPEG_EOI};
struct source_mgr {
    struct jpeg_source_mgr pub;
    const JOCTET* data;
    size_t len;
};

static void init_source(j_decompress_ptr cinfo) {}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
    printf("FILL INPUT BUFFER =====> 1\n");
    source_mgr* src = (source_mgr*)cinfo->src;
    printf("FILL INPUT BUFFER =====> 2\n");
    // no more data. probably an incomplete image; just output EOI
    src->pub.next_input_byte = EOI_BUFFER;
    printf("FILL INPUT BUFFER =====> 3\n");
    src->pub.bytes_in_buffer = 1;
    printf("FILL INPUT BUFFER =====> 4\n");
    return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    source_mgr* src = (source_mgr*)cinfo->src;
    if(src->pub.bytes_in_buffer < num_bytes) {
        // skipping over all of remaining data; output EOI
        src->pub.next_input_byte = EOI_BUFFER;
        src->pub.bytes_in_buffer = 1;
    } else {
        // skipping over only some of the remaining data.
        src->pub.next_input_byte += num_bytes;
        src->pub.bytes_in_buffer -= num_bytes;
    }
}

static void term_source(j_decompress_ptr cinfo) {}

int VPSJpeg::Decode_Jpeg(BYTE* pJpgSrc, int nJpgSrcLen, BYTE* pOut) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned long outLen = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);

    // setup decompress structure
    source_mgr* src = (source_mgr*)cinfo.src;
//    src->pub.init_source = init_source;
    printf("DEBUGGING -----------> 5\n");
    src->pub.fill_input_buffer = fill_input_buffer;
    printf("DEBUGGING -----------> 6\n");
    src->pub.skip_input_data = skip_input_data;
    printf("DEBUGGING -----------> 7\n");
    src->pub.resync_to_restart = jpeg_resync_to_restart;    // default
    printf("DEBUGGING -----------> 8\n");
    src->pub.term_source = term_source;

    // fill the buffers
    src->data = (const JOCTET*)pJpgSrc;
    src->len = nJpgSrcLen;
    src->pub.bytes_in_buffer = nJpgSrcLen;
    src->pub.next_input_byte = src->data;

    printf("DEBUGGING -----------> 9\n");
    // read info from header
    int r = jpeg_read_header(&cinfo, TRUE);
    printf("DEBUGGING -----------> 10\n");
    jpeg_start_decompress(&cinfo);
    printf("DEBUGGING -----------> 11\n");

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_stride = width * cinfo.output_components;

    printf("DECOMPRESS : %d, %d, %d\n", width, height, row_stride);
//    JSAMPARRAY pJpegBuffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);

    return 0;
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
    cinfo.in_color_space = JCS_EXT_BGR;
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

    // if(width == 960 && height == 540) {
    //     Write_to_jpegfile("/Users/hangoon2/onycom/shared/1/test.jpg", pJpgSrc, width, height);

    //     Mat rawData(1, outLen, CV_8SC1, (void*)m_pJpgData);
    //     Mat rawImage = imdecode(rawData, IMREAD_COLOR);

    //     imwrite("/Users/hangoon2/onycom/shared/1/test1.jpg", rawImage);
    // }

    return (int)outLen;
}

VPSJpeg::VPSJpeg() {
    m_pJpgData = (BYTE*)malloc(1024 * 1024);
}

VPSJpeg::~VPSJpeg() {
    free(m_pJpgData);
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
    ULONGLONG ret1 = GetTickCount();
    Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
    ULONGLONG ret2 = GetTickCount();
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);
    ULONGLONG ret3 = GetTickCount();

    Decode_Jpeg(pJpgSrc, nJpgSrcLen, NULL);
    ULONGLONG ret4 = GetTickCount();

    printf("DECOMPRESS TIME(%d, %d) : %lld, %lld\n", width, height, (ret3 - ret2), (ret4 - ret3));

    rotate(rawImage, rawImage, ROTATE_90_COUNTERCLOCKWISE);
    // transpose(rawImage, rawImage);
    // flip(rawImage, rawImage, 0);

    return Encode_Jpeg(rawImage.data, pJpgSrc, quality, height, width);
}

// int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality) {
//     Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
//     Mat rawImage = imdecode(rawData, IMREAD_COLOR);
    
//     transpose(rawImage, rawImage);
//     flip(rawImage, rawImage, 1);
    
//     return Encode(pJpgSrc, rawImage, quality);
// }

int VPSJpeg::RotateRight(BYTE* pJpgSrc, int nJpgSrcLen, int quality, int width, int height) {
    Mat rawData(1, nJpgSrcLen, CV_8SC1, (void*)pJpgSrc);
    Mat rawImage = imdecode(rawData, IMREAD_COLOR);
    
    // transpose(rawImage, rawImage);
    // flip(rawImage, rawImage, 1);

    rotate(rawImage, rawImage, ROTATE_90_CLOCKWISE);
    
    return Encode_Jpeg(rawImage.data, pJpgSrc, quality, height, width);
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