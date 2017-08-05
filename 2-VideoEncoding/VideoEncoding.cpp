/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
*/
#include "VideoEncoding.h"

VideoEncoding::VideoEncoding():
    mCodecCtx(NULL)
{
}


VideoEncoding::~VideoEncoding()
{
    avcodec_free_context(&mCodecCtx);
}

bool VideoEncoding::init()
{
    avcodec_register_all();
    return false;
}

// Configure AVCodecContext parameters Manually
bool VideoEncoding::initCodecContext()
{
    const AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    //const AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!enc) {
        fprintf(stderr, "Failed to find encoder\n");
        return true;
    }

    mCodecCtx = avcodec_alloc_context3(enc);
    if (!mCodecCtx) {
        printf("Failed to allocate the codec context\n");
        return true;
    }

    // The parameters of the video to be encoded
    // can be modefied as required
    mCodecCtx->width = 1280; //equal to the YUV file size
    mCodecCtx->height = 534;
    mCodecCtx->bit_rate = 1000000; // 1Mbps
    mCodecCtx->gop_size = 10;
    mCodecCtx->time_base = { 1, 24 };
    mCodecCtx->framerate = { 24, 1 };
    mCodecCtx->max_b_frames = 1;
    mCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (enc->id == AV_CODEC_ID_H264) {
        //av_opt_set(mCodecCtx->priv_data, "preset", "slow", 0);   // delay ~18 frames
        av_opt_set(mCodecCtx->priv_data, "tune", "zerolatency", 0);    // no delay
    }

    // Initialize mCodecCtx to use the given Codec
    if (avcodec_open2(mCodecCtx, enc, NULL) < 0) {
        printf("Failed to open codec\n");
        return true;
    }

    return false;
}

bool VideoEncoding::readFrameProc(const char * input, const char * output)
{
    FILE *yuvFd = fopen(input, "rb");
    FILE *outFd = fopen(output, "wb");
    if (!outFd || !yuvFd) {
        fprintf(stderr, "Could not open file\n");
        return true;
    }

    AVFrame *frame = NULL;
    if (!(frame = av_frame_alloc())) {
        printf("Failed to allocate video frame\n");
        return true;
    }

    frame->format = mCodecCtx->pix_fmt;
    frame->width = mCodecCtx->width;
    frame->height = mCodecCtx->height;

    if (av_frame_get_buffer(frame, 32) < 0) {
        printf("Failed to allocate the video frame data\n");
        return true;
    }


    int num = 0, i = 0;
    AVPacket pkt;
    // read a frame every time
    while (!feof(yuvFd)) {

        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        if (av_frame_make_writable(frame)) {
            return true;
        }

        // fill frame.data 
        fread(frame->data[0], 1, mCodecCtx->width *mCodecCtx->height, yuvFd);
        fread(frame->data[1], 1, mCodecCtx->width*mCodecCtx->height / 4, yuvFd);
        fread(frame->data[2], 1, mCodecCtx->width*mCodecCtx->height / 4, yuvFd);

        frame->pts = i++;

        // encode video
        avcodec_send_frame(mCodecCtx, frame);
        int ret = avcodec_receive_packet(mCodecCtx, &pkt);

        if (!ret) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, outFd);
            av_packet_unref(&pkt);
        }
        printf("------------------------------------\n");
    }

    // 编码有延时，获取延时的数据

    for (;; i++) {
        avcodec_send_frame(mCodecCtx, NULL);
        int ret = avcodec_receive_packet(mCodecCtx, &pkt);
        if (ret == 0) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
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

    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), outFd);

    fclose(outFd);
    fclose(yuvFd);
    av_frame_free(&frame);

    return false;
}
