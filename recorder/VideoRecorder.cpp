#include "VideoRecorder.h"

#include <unistd.h>
#include <thread>
#include <vector>

using namespace std;

VideoRecorder::VideoRecorder(void* sharedMem) {
    m_isRunning = false;

    m_swsctx = sws_getCachedContext(NULL, 960, 960, AV_PIX_FMT_BGR24, 960, 960,AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    m_rgb32 = av_frame_alloc();
    m_yuv420p = av_frame_alloc();

#if ENABLE_SHARED_MEMORY
    m_sharedMem = sharedMem;
#endif
}

VideoRecorder::~VideoRecorder() {
    if(IsThreadRunning()) {
        StopRecord();

        usleep(100);
    }

    sws_freeContext(m_swsctx);

    av_frame_free(&m_rgb32);
    av_frame_free(&m_yuv420p);
}

void VideoRecorder::StartRecord(int nHpNo, char* filePath) {
    m_inCount = 0;
    m_outCount = 0;

    if(OpenEncoder(filePath)) {
        thread([=]() {
            OnRecord();    
        }).detach();
    }
}

void VideoRecorder::StopRecord()
{
    SetThreadRunning(false);
}

void VideoRecorder::EnQueue(unsigned char* pImgData) {
    if( IsThreadRunning() ) {
        m_inCount++;

        m_recFrame.msec = GetTickCount();

        av_image_fill_arrays(m_rgb32->data, m_rgb32->linesize, (uint8_t*)pImgData, AV_PIX_FMT_BGR24, 960, 960, 1);
        av_image_fill_arrays(m_yuv420p->data, m_yuv420p->linesize, (uint8_t*)m_recFrame.btImg, AV_PIX_FMT_YUV420P, 960, 960, 1);
        sws_scale(m_swsctx, m_rgb32->data, m_rgb32->linesize, 0, 960, m_yuv420p->data, m_yuv420p->linesize);
        
#if ENABLE_SHARED_MEMORY
#else
        m_recQueue.EnQueue(&m_recFrame);
#endif
    }
}

void VideoRecorder::SetThreadRunning(bool isRunning) {
    m_isRunning = isRunning;
}

bool VideoRecorder::IsThreadRunning() {
    return m_isRunning;
}

void VideoRecorder::OnRecord() {
    AVCodecContext* codec_ctx = m_stream->codec;

    // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    int size = av_image_get_buffer_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 16);
    std::vector<uint8_t> framebuf(size);
    av_image_fill_arrays(frame->data, frame->linesize, framebuf.data(), codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1);

    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->format = static_cast<int>(codec_ctx->pix_fmt);

    int ret = avformat_write_header(m_outctx, NULL);
    if(ret < 0) {
        av_frame_free(&frame);
        avcodec_close(codec_ctx);

        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avformat_write_header\n");
        return;
    }

    unsigned nb_frames = 0;
    int got_pkt = 0;
    int startTime = 0;

    AVPacket pkt;
    pkt.data = nullptr;
    pkt.size = 0;
    av_init_packet(&pkt);

    SetThreadRunning(true);

    int count = 0;

    HDCAP* pFrame = (HDCAP*)malloc(sizeof(HDCAP));
    memset(pFrame, 0, sizeof(HDCAP));

    // encoding loop
    while(IsThreadRunning()) {
#if ENABLE_SHARED_MEMORY
        {
#else
        if(m_recQueue.DeQueue(pFrame)) {
#endif
            m_outCount++;

            av_image_fill_arrays(frame->data, frame->linesize, (uint8_t*)pFrame->btImg, AV_PIX_FMT_YUV420P, 960, 960, 1);

            int frameOfGap = 0;
            if(nb_frames > 0) {
                frameOfGap = pFrame->msec - startTime;
            } else {
                startTime = pFrame->msec;
            }

            frame->pts = frameOfGap * codec_ctx->time_base.den / 1000;

            // encode video frame
            ret = avcodec_encode_video2(codec_ctx, &pkt, frame, &got_pkt);
            if(ret < 0) {
                printf("failed avcodec_encode_video2\n");
                break;
            }

            usleep(1);

            // rescale packet timestamp
            pkt.duration = 1;
            pkt.stream_index = m_stream->index;

            av_packet_rescale_ts(&pkt, codec_ctx->time_base, m_stream->time_base);

            // write packet
            ret = av_write_frame(m_outctx, &pkt);
            if(ret >= 0) {
                ++nb_frames;
            }

            av_packet_unref(&pkt);
        }

        this_thread::sleep_for( chrono::milliseconds(1) );
    }

    if(pFrame != NULL) {
        free(pFrame);
        pFrame = NULL;
    }

    av_write_trailer(m_outctx);

    av_frame_free(&frame);
    avcodec_close(codec_ctx);

    avio_close(m_outctx->pb);
    avformat_free_context(m_outctx);

#if ENABLE_SHARED_MEMORY
#else
    m_recQueue.ClearQueue();
#endif
}

bool VideoRecorder::OpenEncoder(char* filePath) {
    int ret = 0;

    m_outctx = avformat_alloc_context();

    AVOutputFormat* fmt = av_guess_format(NULL, filePath, NULL);
    m_outctx->oformat = fmt;

    ret = avio_open2(&m_outctx->pb, filePath, AVIO_FLAG_WRITE, NULL, NULL);
    if(ret < 0) {
        avformat_free_context(m_outctx);

        printf("failed avio_open2\n");
        return false;
    }

    // crate video stream
    m_codec = avcodec_find_encoder(m_outctx->oformat->video_codec);

    m_stream = avformat_new_stream(m_outctx, m_codec);
    if(!m_stream) {
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avformat_new_stream\n");
        return false;
    }

    AVCodecContext* codec_ctx = m_stream->codec;

    codec_ctx->width = 960;
    codec_ctx->height = 960;
    codec_ctx->time_base.num = 1;
    codec_ctx->time_base.den = 16;
    codec_ctx->global_quality = 4;
    codec_ctx->codec_id = fmt->video_codec;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->qmin = 10;
    codec_ctx->qmax = 51;
    codec_ctx->max_b_frames = 3;
//    codec_ctx->bit_rate = 180000;

    if(m_outctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *param = 0;

    if(codec_ctx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
    }

    av_dump_format(m_outctx, 0, filePath, 1);

    // open video encoder
    ret = avcodec_open2(codec_ctx, m_codec, &param);
    if(ret < 0) {
        avcodec_close(codec_ctx);
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avcodec_open2\n");
        return false;
    }

    printf("Success Open Encoder\n");

    return true;
}

void* VideoRecorder::GetFreeInVideoMem(int nHpNo, void* pCap, int maxCount) {

}