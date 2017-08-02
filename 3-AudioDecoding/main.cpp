/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#include "AudioDecoding.h"

int main(int argc, char **argv)
{
    AudioDecoding audioDecoding;

    // play command: ffplay -f s16le -ac 2 -ar 44100 out.pcm
    const char *fileName = "../assets/echo.mp3";

    // play command: ffplay -f f32le -ac 2 -ar 44100 out.pcm
    //const char *fileName = "../assets/echo.aac";

    if (audioDecoding.init(fileName))
        return 1;
    
    if (audioDecoding.initCodecContext())
        return 1;
    if (audioDecoding.readFrameProc())
        return 1;
    
    printf("Save out.pcm successfully!\n");
}