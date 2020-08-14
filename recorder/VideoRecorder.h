#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include "../common/VPSCommon.h"
#include "Rec_Queue.h"

#include <pthread.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

class VideoRecorder {
public:
    VideoRecorder(void* sharedMem, int nHpNo, int retPort);
    virtual ~VideoRecorder();

    void StartRecord(char* filePath);
    void StopRecord();
    bool EnQueue(unsigned char* pImgData);

    void OnRecord();
    void SetThreadRunning(bool isRunning);
    bool IsThreadRunning();

private:
    bool OpenEncoder(char * filePath);

#if ENABLE_SHARED_MEMORY
    void* GetFreeInVideoMem(int iCh, void* pCap, int maxCount);
    void* GetDataInVideoMem(int iCh, void* pCap, int maxCount);
    void ClearVideoMem(int iCh, void* pCap, int maxCount);
    ULONGLONG IncreamentImageId();
#endif

    void RecordStopAndSend();

    bool IsCompressTime(double dlCaptureGap);

private:
    bool m_isRunning;

    pthread_t m_tID;

    int m_nHpNo;

    int m_retPort;  // 결과를 전송할 server port
    BYTE m_pBufSendData[SEND_BUF_SIZE];

    double m_dlCaptureGap;
    double m_dlLastGapTime;

#if ENABLE_SHARED_MEMORY
    void* m_sharedMem;
    ULONGLONG m_i64Img_ID;
#else
    Rec_Queue m_recQueue;
    HDCAP m_recFrame;
#endif

    ULONGLONG m_inCount;
    ULONGLONG m_outCount;

    AVFormatContext* m_outctx;
    AVCodec* m_codec;
    AVStream* m_stream;
    SwsContext* m_swsctx;

    AVFrame* m_rgb32;
    AVFrame* m_yuv420p;
};

#endif 
