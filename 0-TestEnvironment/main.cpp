/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

extern "C" {
#include <libavformat/avformat.h>
};

int main(int argc, char *argv[])
{
    AVFormatContext *fmtCtx = NULL;

    char *inputFile = "../assets/Sample.mkv";

    // Register all components
    av_register_all();

    // Open an input stream and read the header.
    if (avformat_open_input(&fmtCtx, inputFile, NULL, NULL) < 0) {
        printf("Failed to open an input file");
        return -1;
    }

    // Read packets of a media file to get stream information.
    if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    // Print detailed information about the input or output format 
    av_dump_format(fmtCtx, 0, 0, 0);

    avformat_close_input(&fmtCtx);

    return 0;
}