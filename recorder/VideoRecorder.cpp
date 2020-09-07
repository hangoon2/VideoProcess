#include "VideoRecorder.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>

using namespace std;

void* RecordingThreadFunc(void* pArg) {
    VideoRecorder* pRecorder = (VideoRecorder*)pArg;
    pRecorder->OnRecord();

    return NULL;
}

VideoRecorder::VideoRecorder(void* sharedMem, int nHpNo, int retPort) {
    m_isRunning = false;

    m_nHpNo = nHpNo;
    m_retPort = retPort;

    m_swsctx = sws_getCachedContext(NULL, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, 
                                    AV_PIX_FMT_RGB24, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN,
                                    AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    m_rgb24 = av_frame_alloc();
    m_yuv420p = av_frame_alloc();

#if ENABLE_SHARED_MEMORY
    m_sharedMem = sharedMem;
    m_i64Img_ID = 0;
#endif

    m_dlCaptureGap = 1000 / 16;
    m_dlLastGapTime = 0;

    m_pStopRoutine = NULL;

    m_outctx = NULL;
    m_codec_ctx = NULL;
}

VideoRecorder::~VideoRecorder() {
    if(IsThreadRunning()) {
        StopRecord();

        usleep(100);
    }

    sws_freeContext(m_swsctx);

    av_frame_free(&m_rgb24);
    av_frame_free(&m_yuv420p);
}

void VideoRecorder::StartRecord(char* filePath, PVPS_RECORD_STOP_ROUTINE pStopRoutine) {
    m_inCount = 0;
    m_outCount = 0;

    m_pStopRoutine = pStopRoutine;

#if ENABLE_SHARED_MEMORY
    ClearVideoMem(m_nHpNo - 1, m_sharedMem, MEM_SHARED_MAX_COUNT);
#endif

    if( OpenEncoder(filePath) ) { 
        pthread_create(&m_tID, NULL, &RecordingThreadFunc, this);
        if(m_tID == NULL) {
            printf("[VPS:0] Mirroring thread creation fail[%d]\n", errno);
        }
    }
}

void VideoRecorder::StopRecord()
{
    if( IsThreadRunning() ) {
        SetThreadRunning(false);
    }
}

bool VideoRecorder::EnQueue(unsigned char* pImgData) {
    if( IsThreadRunning() && IsCompressTime(m_dlCaptureGap) ) {
        unsigned long time = GetTickCount();

        av_image_fill_arrays(m_rgb24->data, m_rgb24->linesize, (uint8_t*)pImgData, AV_PIX_FMT_RGB24, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, 1);
        
#if ENABLE_SHARED_MEMORY
        HDCAP* pInCap = (HDCAP*)GetFreeInVideoMem(m_nHpNo - 1, m_sharedMem, MEM_SHARED_MAX_COUNT);
        if(pInCap != NULL) {
            m_inCount++;

            pInCap->ui64ID = IncreamentImageId();
            pInCap->msec = time;

            av_image_fill_arrays(m_yuv420p->data, m_yuv420p->linesize, (uint8_t*)pInCap->btImg, AV_PIX_FMT_YUV420P, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, 1);
            sws_scale(m_swsctx, m_rgb24->data, m_rgb24->linesize, 0, MAX_SIZE_SCREEN, m_yuv420p->data, m_yuv420p->linesize);    

            pInCap->accessMode = TYPE_ACCESS_DATA;  // 데이터 있음
        }
#else
        m_inCount++;

        m_recFrame.msec = time;

        av_image_fill_arrays(m_yuv420p->data, m_yuv420p->linesize, (uint8_t*)m_recFrame.btImg, AV_PIX_FMT_YUV420P, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, 1);
        sws_scale(m_swsctx, m_rgb24->data, m_rgb24->linesize, 0, MAX_SIZE_SCREEN, m_yuv420p->data, m_yuv420p->linesize);

        m_recQueue.EnQueue(&m_recFrame);
#endif

        return true;
    }

    return false;
}

void VideoRecorder::SetThreadRunning(bool isRunning) {
    m_isRunning = isRunning;
}

bool VideoRecorder::IsThreadRunning() {
    return m_isRunning;
}

void VideoRecorder::OnRecord() {
//    AVCodecContext* codec_ctx = m_stream->codec;
    AVCodecContext* codec_ctx = m_codec_ctx;

    // allocate frame buffer for encoding
    AVFrame* frame = av_frame_alloc();
    int size = av_image_get_buffer_size(codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 16);
    std::vector<uint8_t> framebuf(size);
    av_image_fill_arrays(frame->data, frame->linesize, framebuf.data(), codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, 1);

    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->format = static_cast<int>(codec_ctx->pix_fmt);

    // int ret = avformat_write_header(m_outctx, NULL);
    // if(ret < 0) {
    //     av_frame_free(&frame);
    //     avcodec_close(codec_ctx);

    //     avio_close(m_outctx->pb);
    //     avformat_free_context(m_outctx);

    //     printf("failed avformat_write_header\n");
    //     return;
    // }

    unsigned nb_frames = 0;
//    int got_pkt = 0;
    int startTime = 0;

    AVPacket pkt;
    pkt.data = nullptr;
    pkt.size = 0;
    av_init_packet(&pkt);

    SetThreadRunning(true);

    HDCAP* pFrame = (HDCAP*)malloc(sizeof(HDCAP));
    memset(pFrame, 0, sizeof(HDCAP));

    // encoding loop
    while( IsThreadRunning() ) {
#if ENABLE_SHARED_MEMORY
        HDCAP* pInCap = (HDCAP*)GetDataInVideoMem(m_nHpNo - 1, m_sharedMem, MEM_SHARED_MAX_COUNT);
        if(pInCap != NULL) {
            pInCap->accessMode = TYPE_ACESS_READING;    // 데이터 읽는 중, 엑세스 금지

            memcpy( pFrame, pInCap, sizeof(HDCAP) );

            pInCap->accessMode = TYPE_ACCESS_EMPTY;

#else
        if(m_recQueue.DeQueue(pFrame)) {
#endif
            av_image_fill_arrays(frame->data, frame->linesize, (uint8_t*)pFrame->btImg, AV_PIX_FMT_YUV420P, MAX_SIZE_SCREEN, MAX_SIZE_SCREEN, 1);

            usleep(1);

            int frameOfGap = 0;
            if(nb_frames > 0) {
                frameOfGap = pFrame->msec - startTime;
            } else {
                startTime = pFrame->msec;
            }

            frame->pts = frameOfGap * codec_ctx->time_base.den / 1000;

            // encode video frame
            // ret = avcodec_encode_video2(codec_ctx, &pkt, frame, &got_pkt);
            // if(ret < 0) {
            //     printf("failed avcodec_encode_video2\n");
            //     break;
            // }

            // this_thread::sleep_for( chrono::milliseconds(1) );

            // // rescale packet timestamp
            // pkt.duration = 1;
            // pkt.stream_index = m_stream->index;

            // av_packet_rescale_ts(&pkt, codec_ctx->time_base, m_stream->time_base);

            // usleep(1);

            // // write packet
            // ret = av_write_frame(m_outctx, &pkt);
            // if(ret >= 0) {
            //     ++nb_frames;
            // }

            Encode(codec_ctx, frame, &pkt);
            ++nb_frames;

            usleep(1);

            av_packet_unref(&pkt);
        }

        this_thread::sleep_for( chrono::milliseconds(1) );
//        usleep(500);
    }

    Encode(codec_ctx, NULL, &pkt);

    printf("[VPS:%d] RECORDING END : %lld, %lld\n", m_nHpNo, m_inCount, m_outCount);

    av_write_trailer(m_outctx);

    printf("[VPS:%d] VideoRecorder::OnRecord >>> 1\n", m_nHpNo);
    if(pFrame != NULL) {
        free(pFrame);
        pFrame = NULL;
        
        printf("[VPS:%d] VideoRecorder::OnRecord >>> 2\n", m_nHpNo);
        if(frame != NULL) {
            av_frame_free(&frame);
            frame = NULL;
        }

        printf("[VPS:%d] VideoRecorder::OnRecord >>> 3\n", m_nHpNo);
    //    avcodec_close(codec_ctx);
        if(m_codec_ctx != NULL) {
            avcodec_free_context(&m_codec_ctx);
            m_codec_ctx = NULL;
        }

        printf("[VPS:%d] VideoRecorder::OnRecord >>> 4\n", m_nHpNo);
        if(m_outctx != NULL) {
            avio_close(m_outctx->pb);
            printf("[VPS:%d] VideoRecorder::OnRecord >>> 5\n", m_nHpNo);
            avformat_free_context(m_outctx);
            m_outctx = NULL;
        }
        printf("[VPS:%d] VideoRecorder::OnRecord >>> 6\n", m_nHpNo);
    }

#if ENABLE_SHARED_MEMORY
#else
    m_recQueue.ClearQueue();
#endif

    if(m_pStopRoutine != NULL) {
        m_pStopRoutine(m_nHpNo);
    } else {
        RecordStopAndSend();
    }
}

void VideoRecorder::Encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt) {
    // 인코딩 명령을 내림. 과거 버전에서는 인코딩 명령에서 결과를 받았지만
    // 최신 버전에서는 명령과 결과 리턴을 따로 처리함 -> 서로 다른 쓰래드에서
    // 명령을 처리할 수 있는 장점이 있음
    int ret = avcodec_send_frame(enc_ctx, frame);
    if(ret < 0) {
        printf("Error sending a frame for encoding\n");
        return;
    }

    while(ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);

        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) {
            return;
        } else if(ret < 0) {
            printf("Error during encoding\n");
            break;
        }

        usleep(1);

        pkt->duration = 1;
        pkt->stream_index = m_stream->index;

        av_packet_rescale_ts(pkt, enc_ctx->time_base, m_stream->time_base);
        
        usleep(1);

        av_write_frame(m_outctx, pkt);
        m_outCount++;

        usleep(1);

        av_packet_unref(pkt);
    }
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

#if 0
    m_stream = avformat_new_stream(m_outctx, m_codec);
#else
    m_stream = avformat_new_stream(m_outctx, NULL);
#endif
    if(!m_stream) {
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avformat_new_stream\n");
        return false;
    }

#if 0
    AVCodecContext* codec_ctx = m_stream->codec;
#else
    m_stream->id = m_outctx->nb_streams - 1;
    m_codec_ctx = avcodec_alloc_context3(m_codec);

    m_stream->time_base.num = 1;
    m_stream->time_base.den = 16;

    AVCodecContext* codec_ctx = m_codec_ctx;
#endif

    codec_ctx->width = MAX_SIZE_SCREEN;
    codec_ctx->height = MAX_SIZE_SCREEN;
    codec_ctx->time_base.num = 1;
    codec_ctx->time_base.den = 16;
    codec_ctx->global_quality = 4;
    codec_ctx->gop_size = 30;
    codec_ctx->codec_id = fmt->video_codec;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->qmin = 10;
    codec_ctx->qmax = 51;
    codec_ctx->max_b_frames = 3;
    codec_ctx->bit_rate = 960000;

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
#if 0
        avcodec_close(codec_ctx);
#else 
        avcodec_free_context(&codec_ctx);
#endif
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avcodec_open2\n");
        return false;
    }

    ret = avcodec_parameters_from_context(m_stream->codecpar, codec_ctx);
    if(ret < 0) {
        avcodec_free_context(&codec_ctx);
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avcodec_parameters_from_context\n");
        return false;
    }

    av_dump_format(m_outctx, 0, filePath, 1);

    ret = avformat_write_header(m_outctx, &param);
    if(ret < 0) {
        avcodec_free_context(&codec_ctx);
        avio_close(m_outctx->pb);
        avformat_free_context(m_outctx);

        printf("failed avformat_write_header\n");
        return false;
    }

    return true;
}

#if ENABLE_SHARED_MEMORY
void* VideoRecorder::GetFreeInVideoMem(int iCh, void* pCap, int maxCount) {
    int iChPerMemCnt = maxCount / MAXCHCNT; // 채널당 몇개의 메모리를 사용하는지 ...
    int iStart = iCh * iChPerMemCnt;
    int iEnd = iStart + iChPerMemCnt;
    HDCAP* pRecCap = (HDCAP*)pCap;

    for(int i = iStart; i < iEnd; i++) {
        if(pRecCap[i].accessMode == TYPE_ACCESS_EMPTY) {
            pRecCap[i].accessMode = TYPE_ACCESS_WRITTING;   // 일단 먼저 예약, 쓰는 중, 엑세스 금지
            return (void*)&pRecCap[i];
        }
    }

    return NULL;
}

void* VideoRecorder::GetDataInVideoMem(int iCh, void* pCap, int maxCount) {
    int iChPerMemCnt = maxCount / MAXCHCNT; // 채널당 몇개의 메모리를 사용하는지 ...
    int iStart = iCh * iChPerMemCnt;
    int iEnd = iStart + iChPerMemCnt;
    HDCAP* pRecCap = (HDCAP*)pCap;

    int iFnd = -1;
    uint64_t iOldID = 0xFFFFFFFFFFFFFFFF;
    for(int i = iStart; i < iEnd; i++) {
        if(pRecCap[i].accessMode == TYPE_ACCESS_DATA) {
            if(pRecCap[i].ui64ID < iOldID) {
                iOldID = pRecCap[i].ui64ID;
                iFnd = i;
            }
        }
    }

    if(iFnd < iStart || iFnd >= iEnd) return NULL;

    return (void*)&pRecCap[iFnd];
}

void VideoRecorder::ClearVideoMem(int iCh, void* pCap, int maxCount) {
    int iChPerMemCnt = maxCount / MAXCHCNT; // 채널당 몇개의 메모리를 사용하는지 ...
    int iStart = iCh * iChPerMemCnt;
    int iEnd = iStart + iChPerMemCnt;
    HDCAP* pRecCap = (HDCAP*)pCap;

    for(int i = iStart; i < iEnd; i++) {
        memset( &pRecCap[i], 0x00, sizeof(HDCAP) );
    }

    m_i64Img_ID = 0;
}

ULONGLONG VideoRecorder::IncreamentImageId() {
    return m_i64Img_ID++;
}
#endif

void VideoRecorder::RecordStopAndSend() {
    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 1\n", m_nHpNo);
    Socket sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET) {
        printf("[VPS:%d] Record Result sock() error[%d]\n", m_nHpNo, errno);
        return;
    }
    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 2\n", m_nHpNo);

    // 종료방식
    // 즉시 연결을 종료한다. 상대방에게는 FIN이나 RTS 시그널이 전달된다.
    short l_onoff = 1, l_linger = 0;    
    linger opt = {l_onoff, l_linger};
    int state = setsockopt( sock, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt) );
    if(state) {
        printf("[VPS:%d] Record Result socket setsockopt() SO_LINGER error\n", m_nHpNo);
        return;
    }

    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 3\n", m_nHpNo);
    struct sockaddr_in servAddr;
    memset( &servAddr, 0x00, sizeof(servAddr) );
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(m_retPort);

    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 4\n", m_nHpNo);
    if( connect( sock, (sockaddr*)&servAddr, sizeof(servAddr) ) == 0 ) {
        int totLen = 0;
        printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 5\n", m_nHpNo);
        BYTE* pSendData = MakeSendData2(CMD_CAPTURE_COMPLETED, m_nHpNo, 0, NULL, (BYTE*)m_pBufSendData, totLen);
        if(pSendData != NULL) {
            int ret = (int)write(sock, pSendData, totLen);
            if(ret == totLen) {
                printf("[VPS:%d] [Record] 단말기 녹화 정지 응답 보냄: 성공\n", m_nHpNo);
            } else {
                printf("[VPS:%d] [Record] 단말기 녹화 정지 응답 보냄: 실패\n", m_nHpNo);
            }
        }
    }

    this_thread::sleep_for( chrono::milliseconds(100) );    // 소켓 데이터 처리를 위한 기다림 ...

    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 6\n", m_nHpNo);
    shutdown(sock, SHUT_RDWR);
    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 7\n", m_nHpNo);
    close(sock);
    printf("[VPS:%d] VideoRecorder::RecordStopAndSend >>> 8\n", m_nHpNo);
}

bool VideoRecorder::IsCompressTime(double dlCaptureGap) {
    double msec = GetTickCount();
    if(msec - m_dlLastGapTime >= dlCaptureGap) {
        m_dlLastGapTime = msec;
        return true;
    }

    return false;
}