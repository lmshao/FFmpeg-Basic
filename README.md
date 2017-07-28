# FFmpeg Basics  ![status](https://img.shields.io/badge/status-unfinished-red.svg?style=flat)
> This is a Visual Studio 2017 solution.   
> FFmpeg version: ffmpeg-3.2.4-win64   

## Introduction
This is the ffmpeg introductory tutorial with code samples. The tutorial focuses on ffmpeg API basic usage, such as encoding, decoding, muxing, demuxing, scaling and transcoding.
## Configure Environment
> This solution has been configured.  
> 此解决方案已经配置好了x64平台开发环境。

Configure VS2017 FFmpeg project.
- **Properties - C/C++ - General - Additional Include Directories**   
\.\./include;%(AdditionalIncludeDirectories)   
- **Properties - Linker - General - Additional Library Directories**
\.\./include;%(AdditionalIncludeDirectories)
- **Properties - Linker - Input - Additional Dependencies**   
avcodec.lib;avformat.lib;avutil.lib;swscale.lib;swresample.lib;postproc.lib;avfilter.lib;avdevice.lib;%(AdditionalDependencies)  

配置VS2017 FFmpeg开发环境
- **属性 - C/C++ - 常规 - 附加包含目录**
\.\./include;%(AdditionalIncludeDirectories)
- **属性 - 链接器 - 常规 - 附加库目录**
\.\./lib;%(AdditionalLibraryDirectories)
- **属性 - 链接器 - 输入 - 附加依赖项**
avcodec.lib;avformat.lib;avutil.lib;swscale.lib;swresample.lib;postproc.lib;avfilter.lib;avdevice.lib;%(AdditionalDependencies)   

## Project Description

### 0. Test Environment   
读取视频文件Sample.mkv，输出文件信息。

### 1. Video Decoding
读取视频文件Sample.mkv，提取视频流，解码为YUV420P像素格式，并保存为原始YUV420P格式视频文件Sample.yuv。   

### 2. Video Encoding
读取未压缩的YUV420P视频文件Sample.yuv，编码为H.264视频格式，并保存为H.264裸流文件Sample.h264。   

### 3. Audio Decoding
读取视频文件Sample.mkv，提取音频流，解码为PCM格式，并保存为Sample.wav。   

### 4. Audio Encoding
读取视频文件Sample.wav，编码为MP3格式，并保存为Sample.mp3。   

### 5. Remuxing
读取视频文件Sample.mkv，解复用抽取媒体流，封装为MP4容器格式文件Sample.mp4。    

Changing the "container" format used for a given file. For example from MKV to TS or from MP4 to MKV.   

### 6. Tanscoding
读取视频文件Sample.mkv，解复用抽取音视流，分别编码为HEVC/H.265视频流和OGG音频流，并封装为Sample.ts。   
