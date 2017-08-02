/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
};

class AudioDecoding
{
public:
    AudioDecoding();
    ~AudioDecoding();
    bool init(const char *file);
    bool initCodecContext();
    bool readFrameProc();

private:
    AVFormatContext *mFormatCtx;
    AVCodecContext  *mCodecCtx;
};