#include "AudioDecoding.h"



AudioDecoding::AudioDecoding():
    mFormatCtx(NULL), mCodecCtx(NULL)
{
}


AudioDecoding::~AudioDecoding()
{
    avcodec_free_context(&mCodecCtx);
    avformat_close_input(&mFormatCtx);
}

bool AudioDecoding::init(const char * file)
{
    av_register_all();

    if ((avformat_open_input(&mFormatCtx, file, 0, 0)) < 0) {
        printf("Failed to open input file\n");
        return true;
    }

    if ((avformat_find_stream_info(mFormatCtx, 0)) < 0) {
        printf("Failed to retrieve input stream information\n");
        return true;
    }

    av_dump_format(mFormatCtx, 0, file, 0);
    return false;
}

bool AudioDecoding::initCodecContext()
{
    // Find a decoder with a matching codec ID
    AVCodec *dec = avcodec_find_decoder(mFormatCtx->streams[0]->codecpar->codec_id);
    if (!dec) {
        printf("Failed to find codec!\n");
        return true;
    }

    // Allocate a codec context for the decoder
    if (!(mCodecCtx = avcodec_alloc_context3(dec))) {
        printf("Failed to allocate the codec context\n");
        return true;
    }

#if 1 
    // need?
    // Fill the codec contex based on the supplied codec parameters.
    if (avcodec_parameters_to_context(mCodecCtx, mFormatCtx->streams[0]->codecpar) < 0) {
        printf("Failed to copy codec parameters to decoder context!\n");
        return true;
    }
#endif

    // Initialize the AVCodecContext to use the given Codec
    if (avcodec_open2(mCodecCtx, dec, NULL) < 0) {
        printf("Failed to open codec\n");
        return true;
    }
    return false;
}

bool AudioDecoding::readFrameProc()
{

    FILE *fd = fopen("out.pcm", "wb");
    if (!fd) {
        printf("Failed to open input file\n");
        return true;
    }
    
    AVPacket packet;
    av_init_packet(&packet);

    AVFrame *frame = av_frame_alloc();

    while (int num = av_read_frame(mFormatCtx, &packet) >= 0) {

        avcodec_send_packet(mCodecCtx, &packet);
        int ret = avcodec_receive_frame(mCodecCtx, frame);
        if (!ret) {

            // number of bytes per sample, 16bit is 2 Bytes
            int sampleBytes = av_get_bytes_per_sample(mCodecCtx->sample_fmt);

            // write audio file for Planar sample format
            for (int i = 0; i < frame->nb_samples; i++)
                for (int ch = 0; ch < mCodecCtx->channels; ch++)
                    fwrite(frame->data[ch] + sampleBytes*i, 1, sampleBytes, fd);

            // write audio file for Packed sample format
            // TODO
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);

    fclose(fd);

    return false;
}
