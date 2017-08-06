/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#include "AudioEncoding.h"

AudioEncoding::AudioEncoding()
    :mCodecCtx(NULL)
{
}


AudioEncoding::~AudioEncoding()
{
    avcodec_free_context(&mCodecCtx);
}

bool AudioEncoding::init()
{
    avcodec_register_all();
    return false;
}

bool AudioEncoding::initCodecContext()
{
    const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!enc) {
        printf("Failed to find codec\n");
    }

    mCodecCtx = avcodec_alloc_context3(enc);
    if (!mCodecCtx) {
        printf("Failed to allocate the codec context\n");
        return true;
    }

    mCodecCtx->bit_rate = 128000;
    mCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16P;
    mCodecCtx->sample_rate = 44100;
    mCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    mCodecCtx->channels = 2;

    if (avcodec_open2(mCodecCtx, enc, NULL) < 0) {
        printf("Failed to open encodec\n");
        return true;
    }
    return false;
}

bool AudioEncoding::readFrameProc(const char * input, const char * output)
{
    FILE *pcmFd = fopen(input, "rb");
    FILE *outFd = fopen(output, "wb");
    if (!outFd || !pcmFd) {
        fprintf(stderr, "Could not open file\n");
        return true;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        printf("Failed to allocate video frame\n");
        return true;
    }

    frame->format = mCodecCtx->sample_fmt;
    frame->nb_samples = mCodecCtx->frame_size;
    frame->channel_layout = mCodecCtx->channel_layout;

    if (av_frame_get_buffer(frame, 0) < 0) {
        printf("Failed to allocate the video frame data\n");
        return true;
    }

    int num = 0;
    AVPacket pkt;
    // read a frame every time
    while (!feof(pcmFd)) {

        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        if (av_frame_make_writable(frame)) {
            return true;
        }
        int sampleBytes = av_get_bytes_per_sample(mCodecCtx->sample_fmt);

        // AV_SAMPLE_FMT_S16P
        // fill frame.data 
        for (int i = 0; i < frame->nb_samples; i++)
            for (int ch = 0; ch < mCodecCtx->channels; ch++)
                fread(frame->data[ch] + i*sampleBytes, 1, sampleBytes, pcmFd);

        // encode audio
        avcodec_send_frame(mCodecCtx, frame);
        int ret = avcodec_receive_packet(mCodecCtx, &pkt);

        if (!ret) {
            printf("Write frame %3d (size=%5d)\n", num++, pkt.size);
            fwrite(pkt.data, 1, pkt.size, outFd);
            av_packet_unref(&pkt);
        }
    }

    printf("------------- get delayed data --------------------\n");

    // get delayed data
    for (;;) {
        avcodec_send_frame(mCodecCtx, NULL);
        int ret = avcodec_receive_packet(mCodecCtx, &pkt);
        if (ret == 0) {
            printf("Write frame %3d (size=%5d)\n", num++, pkt.size);
            fwrite(pkt.data, 1, pkt.size, outFd);
            av_packet_unref(&pkt);
        }
        else if (ret == AVERROR_EOF) {
            printf("Write frame complete\n");
            break;
        }
        else {
            printf("Error encoding frame\n");
            break;
        }

    }

    fclose(outFd);
    fclose(pcmFd);
    av_frame_free(&frame);

    return false;

    return false;
}

