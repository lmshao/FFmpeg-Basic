/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#include "Remuxing.h"

Remuxing::Remuxing():
    inFmtCtx(NULL), outFmtCtx(NULL)
{
}

Remuxing::~Remuxing()
{
    avformat_close_input(&inFmtCtx);
    avio_closep(&outFmtCtx->pb);
    avformat_free_context(outFmtCtx);
}

bool Remuxing::init(const char *infile, const char *outfile)
{
    av_register_all();

    if (avformat_open_input(&inFmtCtx, infile, 0, 0) < 0) {
        printf("Failed to open input file '%s'", infile);
        return true;

    }

    if (avformat_find_stream_info(inFmtCtx, 0) < 0) {
        printf("Failed to retrieve input stream information");
        return true;
    }

    av_dump_format(inFmtCtx, 0, infile, 0);
    
    mOutFile = outfile;

    return false;
}

bool Remuxing::initOutFmtCtx()
{
    // Allocate an AVFormatContext for an output format
    avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, mOutFile);
    if (!outFmtCtx) {
        printf("Failed to create output context\n");
        return true;
    }

    mStreamNum = inFmtCtx->nb_streams;
    mStreamMap = (int *)av_mallocz_array(mStreamNum, sizeof(mStreamMap));
    if (!mStreamMap) {
        return true;
    }

    int stream_index = 0;

    // Map media stream
    for (int i = 0; i < (int)inFmtCtx->nb_streams; i++) {

        AVCodecParameters *inCodecPar = inFmtCtx->streams[i]->codecpar;

        // Non-media stream
        if (inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
            inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            mStreamMap[i] = -1;
            continue;
        }

        mStreamMap[i] = stream_index++;
        
        // Add a new stream to a media file
        AVStream *outStream = avformat_new_stream(outFmtCtx, NULL);
        if (!outStream) {
            printf("Failed to allocating output stream\n");
        }

        // copy the contents of src to dst
        if (avcodec_parameters_copy(outStream->codecpar, inCodecPar) < 0) {
            printf("Failed to copy codec parameters\n");
        }
        outStream->codecpar->codec_tag = 0;
    }

    av_dump_format(outFmtCtx, 0, mOutFile, 1);

    return false;
}

bool Remuxing::transferMediaStream()
{
    AVOutputFormat *ofmt = outFmtCtx->oformat;

    if (!(ofmt->flags & AVFMT_NOFILE)) {

        if (avio_open(&outFmtCtx->pb, mOutFile, AVIO_FLAG_WRITE) < 0) {
            printf("Failed to open output file '%s'", mOutFile);
        }
    }

    // write header
    if (avformat_write_header(outFmtCtx, NULL)) {
        printf("Error occurred when opening output file\n");
    }

    
    // write contents
    
    AVPacket pkt;

    while (1) {
        AVStream *inStream, *outStream;

        if (av_read_frame(inFmtCtx, &pkt) < 0)
            break;

        inStream = inFmtCtx->streams[pkt.stream_index];

        // 正常的媒体流
        if (pkt.stream_index >= mStreamNum || mStreamMap[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = mStreamMap[pkt.stream_index];
        outStream = outFmtCtx->streams[pkt.stream_index];

        // copy packet
        pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
        pkt.pos = -1;

        if (av_interleaved_write_frame(outFmtCtx, &pkt) < 0) {
            printf("Error muxing packet\n");
            break;
        }
        av_packet_unref(&pkt);
    }

    // write trailer
    av_write_trailer(outFmtCtx);

    return false;
}
