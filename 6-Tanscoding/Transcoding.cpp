/*
 * Copyright (c) 2017 Liming Shao <lmshao@163.com>
 */

#include "Transcoding.h"


Transcoding::Transcoding():
    iFmtCtx(NULL), oFmtCtx(NULL)
{
}

Transcoding::~Transcoding()
{
}

bool Transcoding::initSys()
{
    av_register_all();

    // Initialize the filter system.
    avfilter_register_all();

    return false;
}

bool Transcoding::initDecCtx(const char * infile)
{
    if (avformat_open_input(&iFmtCtx, infile, NULL, NULL) < 0) {
        printf("Failed to open input file\n");
        return true;
    }

    if (avformat_find_stream_info(iFmtCtx, NULL) < 0) {
        printf("Failed to find stream information\n");
        return true;
    }

    av_dump_format(iFmtCtx, 0, infile, 0);

    streamCtx = (StreamContext*)av_mallocz_array(iFmtCtx->nb_streams, sizeof(StreamContext));
    if (!streamCtx)
        return true;

    for (unsigned int i = 0; i < iFmtCtx->nb_streams; i++) {
        AVStream *stream = iFmtCtx->streams[i];

        // Find decoder
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!dec) {
            printf("Failed to find decoder for stream #%u\n", i);
            return true;
        }

        // Allocate codec context
        AVCodecContext *decCtx = avcodec_alloc_context3(dec);
        if (!decCtx) {
            printf("Failed to allocate the decoder context for stream #%u\n", i);
            return true;
        }

        // Fill the codec context
        if (avcodec_parameters_to_context(decCtx, stream->codecpar) < 0) {
            printf("Failed to copy decoder parameters\n");
            return true;
        }
        /* Reencode video & audio and remux subtitles etc. */
        if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO
            || decCtx->codec_type == AVMEDIA_TYPE_AUDIO) {

            if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO)
                decCtx->framerate = av_guess_frame_rate(iFmtCtx, stream, NULL);

            // Open decoder
            if (avcodec_open2(decCtx, dec, NULL) < 0) {
                printf("Failed to open decoder for stream #%u\n", i);
                return true;
            }
        }
        streamCtx[i].decCtx = decCtx;
    }

    return false;
}

bool Transcoding::initEncCtx(const char *outfile)
{
    avformat_alloc_output_context2(&oFmtCtx, NULL, NULL, outfile);
    if (!oFmtCtx) {
        printf("Failed to create output context\n");
        return true;
    }

    AVCodecContext *decCtx;

    for (unsigned int i = 0; i < iFmtCtx->nb_streams; i++) {
        // Add new stream
        AVStream *outStream = avformat_new_stream(oFmtCtx, NULL);
        if (!outStream) {
            printf("Failed to allocate output stream\n");
            return true;
        }

        AVStream *inStream = iFmtCtx->streams[i];
        decCtx = streamCtx[i].decCtx;

        if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO
            || decCtx->codec_type == AVMEDIA_TYPE_AUDIO) {

            /* in this example, we choose transcoding to same codec */
            // Find encoder
            AVCodec *enc = avcodec_find_encoder(decCtx->codec_id);
            if (!enc) {
                printf("Necessary encoder not found\n");
                return true;
            }

            // Allocate codec context
            AVCodecContext *encCtx = avcodec_alloc_context3(enc);
            if (!encCtx) {
                printf("Failed to allocate the encoder context\n");
                return AVERROR(ENOMEM);
            }

            /* In this example, we transcode to same properties (picture size,
            * sample rate etc.). These properties can be changed for output
            * streams easily using filters */

            // Video Stream
            if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO) {

                encCtx->height = decCtx->height;
                encCtx->width = decCtx->width;
                encCtx->sample_aspect_ratio = decCtx->sample_aspect_ratio;

                // take first format from list of supported formats
                if (enc->pix_fmts)
                    encCtx->pix_fmt = enc->pix_fmts[0];
                else
                    encCtx->pix_fmt = decCtx->pix_fmt;

                encCtx->time_base = av_inv_q(decCtx->framerate);
            }
            // Audio Stream
            else {
                encCtx->sample_rate = decCtx->sample_rate;
                encCtx->channel_layout = decCtx->channel_layout;
                encCtx->channels = av_get_channel_layout_nb_channels(encCtx->channel_layout);

                // take first format from list of supported formats
                encCtx->sample_fmt = enc->sample_fmts[0];
                encCtx->time_base = { 1, encCtx->sample_rate };
            }

            // Video & Audio Stream
            if (avcodec_open2(encCtx, enc, NULL) < 0) {
                printf("Failed to open video encoder for stream #%u\n", i);
                return true;
            }

            if (avcodec_parameters_from_context(outStream->codecpar, encCtx) < 0) {
                printf("Failed to copy encoder parameters to output stream #%u\n", i);
                return true;
            }

            if (oFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
                encCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            outStream->time_base = encCtx->time_base;
            streamCtx[i].encCtx = encCtx;
        }
        // Unknown Stream
        else if (decCtx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            printf("Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return true;
        }
        // Other Stream needed to be remuxed
        else {
            // Copy parameters
            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
                printf("Copying parameters for stream #%u failed\n", i);
                return true;
            }
            outStream->time_base = inStream->time_base;
        }

    }

    av_dump_format(oFmtCtx, 0, outfile, 1);

    // Open output file
    if (!(oFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&oFmtCtx->pb, outfile, AVIO_FLAG_WRITE) < 0) {
            printf("Failed to open output file '%s'", outfile);
            return true;
        }
    }

    // Write output file header
    if (avformat_write_header(oFmtCtx, NULL) < 0) {
        printf("Error occurred when opening output file\n");
        return true;
    }

    return false;
}

bool Transcoding::initFilters()
{
    const char *filterSpec;
    int ret;
    filterCtx = (FilteringContext*)av_malloc_array(iFmtCtx->nb_streams, sizeof(FilteringContext));
    if (!filterCtx)
        return AVERROR(ENOMEM);

    for (unsigned int i = 0; i < iFmtCtx->nb_streams; i++) {

        filterCtx[i].buffSrcCtx = NULL;
        filterCtx[i].buffSinkCtx = NULL;
        filterCtx[i].filterGraph = NULL;

        // ÅÅ³ý·ÇÒôÊÓÆµÁ÷
        if (!(iFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
            || iFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
            continue;


        if (iFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            filterSpec = "null"; /* passthrough (dummy) filter for video */
        else
            filterSpec = "anull"; /* passthrough (dummy) filter for audio */
        ret = this->initFilter(&filterCtx[i], streamCtx[i].decCtx, streamCtx[i].encCtx, filterSpec);
        if (ret)
            return ret;
    }

    return false;
}

bool Transcoding::filterEncodeWriteFrame(AVFrame * frame, unsigned int streamIndex)
{
    AVFrame *filtFrame;

    printf("Pushing decoded frame to filters\n");

    // Push the decoded frame into the filtergraph
    if (av_buffersrc_add_frame_flags(filterCtx[streamIndex].buffSrcCtx, frame, 0) < 0) {
        printf("Failed to feed the filtergraph\n");
        return true;
    }

    // Pull filtered frames from the filtergraph
    for (;;) {
        if (!(filtFrame = av_frame_alloc())) {
            printf("Failed to allocate AVFrame\n");
            return true;
        }

        printf("Pulling filtered frame from filters\n");
        int ret = 0;
        // Get a frame with filtered data from sink and put it in frame.
        if ((ret = av_buffersink_get_frame(filterCtx[streamIndex].buffSinkCtx, filtFrame)) < 0) {
            printf("Failed to get a frame with filtered data\n");
            av_frame_free(&filtFrame);
            break;
        }

        filtFrame->pict_type = AV_PICTURE_TYPE_NONE;
        if (encodeWriteFrame(filtFrame, streamIndex, NULL))
            break;
    }

    return false;
}

bool Transcoding::flushEncoder(unsigned int streamIndex)
{
    if (!(streamCtx[streamIndex].encCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return false;

    while (1) {
        printf("Flushing stream #%u encoder\n", streamIndex);
        if (encodeWriteFrame(NULL, streamIndex, NULL))
            break;
    }

    return false;
}

bool Transcoding::transcode()
{
    unsigned int streamIndex;

    AVPacket packet;
    packet.data = NULL;
    packet.size = 0;

    AVFrame *frame = av_frame_alloc();


    while (av_read_frame(iFmtCtx, &packet) >= 0) {

        streamIndex = packet.stream_index;
        //type = iFmtCtx->streams[streamIndex]->codecpar->codec_type;
        printf("Demuxer gave frame of stream_index %u\n", streamIndex);


        if (filterCtx[streamIndex].filterGraph) {
            printf("Going to reencode & filter the frame\n");

            // Convert valid timing fields
            av_packet_rescale_ts(&packet, iFmtCtx->streams[streamIndex]->time_base, streamCtx[streamIndex].decCtx->time_base);

            avcodec_send_packet(streamCtx[streamIndex].decCtx, &packet);
            if (avcodec_receive_frame(streamCtx[streamIndex].decCtx, frame))
                break;
            
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            filterEncodeWriteFrame(frame, streamIndex);
        }
        // remux this frame without reencoding
        else {

            av_packet_rescale_ts(&packet, iFmtCtx->streams[streamIndex]->time_base, oFmtCtx->streams[streamIndex]->time_base);

            // Write output media file
            if (av_interleaved_write_frame(oFmtCtx, &packet)) {
                printf("Failed to write a packet\n");
                break;
            }
        }

        av_packet_unref(&packet);
    }

    // flush filters and encoders
    for (unsigned int i = 0; i < iFmtCtx->nb_streams; i++) {
        
        // flush filter
        if (!filterCtx[i].filterGraph)
            continue;

        if (filterEncodeWriteFrame(NULL, i)) {
            printf("Flushing filter failed\n");
            return true;
        }

        // flush encoder */
        flushEncoder(i);
    }

    // Write trailer
    av_write_trailer(oFmtCtx);

    return false;
}

bool Transcoding::initFilter(FilteringContext * fctx, AVCodecContext * decCtx, AVCodecContext * encCtx, const char * filterSpec)
{
    char args[512];
    int ret = 0;
    AVFilter *buffersrc = NULL;
    AVFilter *buffersink = NULL;
    AVFilterContext *buffSrcCtx = NULL;
    AVFilterContext *buffSinkCtx = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filterGraph) {
        ret = AVERROR(ENOMEM);
        return true;
    }

    // get AVFilterContext 
    if (decCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            //return true;
        }

        snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            decCtx->width, decCtx->height, decCtx->pix_fmt,
            decCtx->time_base.num, decCtx->time_base.den,
            decCtx->sample_aspect_ratio.num,
            decCtx->sample_aspect_ratio.den);

        // Create and add a filter instance into an existing graph.
        ret = avfilter_graph_create_filter(&buffSrcCtx, buffersrc, "in", args, NULL, filterGraph);
        if (ret < 0) {
            printf("Cannot create buffer source\n");
            return true;
        }

        ret = avfilter_graph_create_filter(&buffSinkCtx, buffersink, "out", NULL, NULL, filterGraph);
        if (ret < 0) {
            printf(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            return true;
        }

        ret = av_opt_set_bin(buffSinkCtx, "pix_fmts", (uint8_t*)&encCtx->pix_fmt, sizeof(encCtx->pix_fmt), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            return true;
        }
    }
    else if (decCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            printf("filtering source or sink element not found\n");
            return true;
        }

        if (!decCtx->channel_layout)
            decCtx->channel_layout = av_get_default_channel_layout(decCtx->channels);

        snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%0llx",
            decCtx->time_base.num, decCtx->time_base.den, decCtx->sample_rate,
            av_get_sample_fmt_name(decCtx->sample_fmt),
            (unsigned long long)decCtx->channel_layout);

        // Create and add a filter instance into an existing graph.
        ret = avfilter_graph_create_filter(&buffSrcCtx, buffersrc, "in", args, NULL, filterGraph);
        if (ret < 0) {
            printf("Cannot create audio buffer source\n");
            return true;
        }

        ret = avfilter_graph_create_filter(&buffSinkCtx, buffersink, "out", NULL, NULL, filterGraph);
        if (ret < 0) {
            printf("Cannot create audio buffer sink\n");
            return true;
        }

        ret = av_opt_set_bin(buffSinkCtx, "sample_fmts", (uint8_t*)&encCtx->sample_fmt, sizeof(encCtx->sample_fmt), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            printf("Cannot set output sample format\n");
            return true;
        }

        ret = av_opt_set_bin(buffSinkCtx, "channel_layouts", (uint8_t*)&encCtx->channel_layout, sizeof(encCtx->channel_layout), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            printf("Cannot set output channel layout\n");
            return true;
        }

        ret = av_opt_set_bin(buffSinkCtx, "sample_rates", (uint8_t*)&encCtx->sample_rate, sizeof(encCtx->sample_rate), AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            printf("Cannot set output sample rate\n");
            return true;
        }
    }
    else {
        return true;
    }

    // Fill AVFilterInOut
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffSrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffSinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if (!outputs->name || !inputs->name) {
        return true;
    }

    // get filterGraph
    // Add a graph described by a string to a graph.
    if ((ret = avfilter_graph_parse_ptr(filterGraph, filterSpec, &inputs, &outputs, NULL)) < 0)
        return true;

    // Check validity and configure all the links and formats in the graph.
    if (avfilter_graph_config(filterGraph, NULL) < 0) {
        return true;
    }

    // Fill Filtering Context
    fctx->buffSrcCtx = buffSrcCtx;
    fctx->buffSinkCtx = buffSinkCtx;
    fctx->filterGraph = filterGraph;

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return false;
}

bool Transcoding::encodeWriteFrame(AVFrame * filtFrame, unsigned int streamIndex, int * got_frame)
{   
    AVPacket encPkt;
    encPkt.data = NULL;
    encPkt.size = 0;
    av_init_packet(&encPkt);

    //printf("Encoding frame\n");

    avcodec_send_frame(streamCtx[streamIndex].encCtx, filtFrame);
    int ret = 0;
    if ( (ret = avcodec_receive_packet(streamCtx[streamIndex].encCtx, &encPkt))) {
        printf("Failed to read encoded packet\n");
        return true;
    }

    av_frame_free(&filtFrame);
    
    // prepare packet for muxing

    encPkt.stream_index = streamIndex;
    // Convert valid timing fields
    av_packet_rescale_ts(&encPkt, streamCtx[streamIndex].encCtx->time_base, oFmtCtx->streams[streamIndex]->time_base);

    printf("Muxing frame---------------------------\n");

    // Write output media file
    if (av_interleaved_write_frame(oFmtCtx, &encPkt)) {
        printf("Failed to write a packet to an output media file\n");
        return true;
    }

    return false;
}
