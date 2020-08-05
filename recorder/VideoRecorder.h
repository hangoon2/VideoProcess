#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include "../common/VPSCommon.h"
#include "Rec_Queue.h"

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
    VideoRecorder(void* sharedMem);
    virtual ~VideoRecorder();

    void StartRecord(int nHpNo, char* filePath);
    void StopRecord();
    void EnQueue(unsigned char* pImgData);

    void OnRecord();
    void SetThreadRunning(bool isRunning);
    bool IsThreadRunning();

private:
    bool OpenEncoder(char * filePath);
    void* GetFreeInVideoMem(int nHpNo, void* pCap, int maxCount);

private:
    bool m_isRunning;

#if ENABLE_SHARED_MEMORY
    void* m_sharedMem;
#else
    Rec_Queue m_recQueue;
#endif

    HDCAP m_recFrame;

    int m_inCount;
    int m_outCount;

    int m_lastMsec;
    int m_lastRecMsec;

    AVFormatContext* m_outctx;
    AVCodec* m_codec;
    AVStream* m_stream;
    SwsContext* m_swsctx;

    AVFrame* m_rgb32;
    AVFrame* m_yuv420p;
};

#endif 
