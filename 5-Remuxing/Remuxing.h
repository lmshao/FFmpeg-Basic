/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avformat.h>
};

class Remuxing
{
public:
    Remuxing();
    ~Remuxing();
    bool init(const char *infile, const char *outfile);
    bool initOutFmtCtx();
    bool transferMediaStream();

private:
    AVFormatContext *inFmtCtx;
    AVFormatContext *outFmtCtx;
    int mStreamNum;
    int *mStreamMap;
    const char *mOutFile;
};

