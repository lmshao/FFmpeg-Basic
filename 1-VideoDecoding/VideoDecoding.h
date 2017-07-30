/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoDecoding
{
public:
    VideoDecoding();
    ~VideoDecoding();
    bool init(const char *file);
    bool findStreamIndex();
    bool initCodecContext();
    bool readFrameProc();

private:
    bool decodeVideoFrame(AVPacket *pkt, AVFrame *fram, FILE *fd);
    bool savePGM(AVFrame *frame);
    bool saveYUV(AVFrame *frame, FILE *fd);

    int mVideoStreamIndex;
    AVFormatContext *mFormatCtx;
    AVCodecContext  *mCodecCtx;
};


