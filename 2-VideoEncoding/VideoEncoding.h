/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

class VideoEncoding
{
public:
    VideoEncoding();
    ~VideoEncoding();
    bool init();
    bool initCodecContext();
    bool readFrameProc(const char *input, const char *output);

private:
    AVCodecContext  *mCodecCtx;
};
