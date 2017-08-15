/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
};


typedef struct FilteringContext {
    AVFilterContext *buffSinkCtx;
    AVFilterContext *buffSrcCtx;
    AVFilterGraph *filterGraph;
}FilteringContext;

typedef struct StreamContext {
    AVCodecContext *decCtx;
    AVCodecContext *encCtx;
}StreamContext;

class Transcoding
{
public:
    Transcoding();
    ~Transcoding();
    bool initSys();
    bool initDecCtx(const char *infile);
    bool initEncCtx(const char *outfile);
    bool initFilters();
    bool filterEncodeWriteFrame(AVFrame *frame, unsigned int streamIndex);
    bool flushEncoder(unsigned int streamIndex);
    bool transcode();


private:
    bool initFilter(FilteringContext* fctx, AVCodecContext *decCtx, AVCodecContext *encCtx, const char *filterSpec);
    bool encodeWriteFrame(AVFrame *filtFrame, unsigned int streamIndex, int *got_frame);
    

    AVFormatContext *iFmtCtx;
    AVFormatContext *oFmtCtx;
    StreamContext *streamCtx;
    FilteringContext *filterCtx;
};

