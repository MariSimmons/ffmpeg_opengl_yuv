#include "Decoder.h"

#include <memory>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>

#include <QDebug>
#include <QThread>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libswscale/swscale.h>
}

#include "OpenGLDisplayYUV.h"

class DecoderPrivate
{
    Q_DECLARE_PUBLIC(Decoder)

    DecoderPrivate(OpenGLDisplayYUV *display, Decoder* ownerPtr)
        : q_ptr{ownerPtr}
        , display{display}
        , format_ctx{avformat_alloc_context()}
    {
        // Register everything
        av_register_all();
        avformat_network_init();
    }

    Decoder * const     q_ptr;
    OpenGLDisplayYUV*   display;
    std::atomic_bool    isStop {false};
    AVPixelFormat       pix_format;

    // ------ ffmpeg vodoo ------
    AVFormatContext* format_ctx = nullptr;

    AVCodecContext* codec_ctx = nullptr;
    int video_stream_index;

    // ----

    AVStream* stream = nullptr;
    AVCodec *codec = nullptr;
    AVFormatContext* output_ctx = nullptr;
    int cnt = 0;
};

Decoder::Decoder(OpenGLDisplayYUV *display, QObject *parent)
    : QObject(parent)
    , d_ptr{new DecoderPrivate(display, this)}
{ }

Decoder::~Decoder()
{ }

bool Decoder::openRTSP(const QString& rtspUrl)
{
    Q_D(Decoder);

    //open RTSP
    if (avformat_open_input(&d->format_ctx, rtspUrl.toStdString().c_str(), nullptr, nullptr) != 0) {
        return false;
    }

    if (avformat_find_stream_info(d->format_ctx, nullptr) < 0) {
        return false;
    }

    //search video stream
    for (unsigned i = 0; i < d->format_ctx->nb_streams; i++)
    {
        if (d->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            d->video_stream_index = i;
    }


    //open output
    d->output_ctx = avformat_alloc_context();

    //start reading packets from stream and write them to file
    av_read_play(d->format_ctx);    //play RTSP

    // Get the codec
    d->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!d->codec) {
        return false;
    }

    // Add this to allocate the context by codec
    d->codec_ctx = avcodec_alloc_context3(d->codec);

    avcodec_get_context_defaults3(d->codec_ctx, d->codec);
    avcodec_copy_context(d->codec_ctx, d->format_ctx->streams[d->video_stream_index]->codec);

    if (avcodec_open2(d->codec_ctx, d->codec, nullptr) < 0)
        return false;

    switch (d->codec_ctx->pix_fmt)
    {
        case AV_PIX_FMT_YUVJ420P:
          d->pix_format = AV_PIX_FMT_YUV420P;
          break;
        case AV_PIX_FMT_YUVJ422P:
          d->pix_format = AV_PIX_FMT_YUV422P;
          break;
        case AV_PIX_FMT_YUVJ444P:
          d->pix_format = AV_PIX_FMT_YUV444P;
          break;
        case AV_PIX_FMT_YUVJ440P:
          d->pix_format = AV_PIX_FMT_YUV440P;
          break;
        default:
          d->pix_format = d->codec_ctx->pix_fmt;
    }

    return true;
}

void Decoder::run()
{
    Q_D(Decoder);

    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, d->codec_ctx->width, d->codec_ctx->height);

    uint8_t* picture_buffer = (uint8_t*) (av_malloc(size));
    AVFrame* picture = av_frame_alloc();
    int size2 = avpicture_get_size(AV_PIX_FMT_RGB24, d->codec_ctx->width, d->codec_ctx->height);

    avpicture_fill((AVPicture *) picture, picture_buffer, AV_PIX_FMT_YUV420P,
            d->codec_ctx->width, d->codec_ctx->height);


    AVPacket packet;
    av_init_packet(&packet);

    while (av_read_frame(d->format_ctx, &packet) >= 0 && !d->isStop)
    {
//        qInfo() << "Frame: " << d->cnt;
        if (packet.stream_index == d->video_stream_index)
        {
//            packet is video
//            qInfo() << "2 Is Video";
            if (d->stream == nullptr)
            {
//                create stream
//                 qInfo() << "3 create stream";
                d->stream = avformat_new_stream(d->output_ctx,
                                                d->format_ctx->streams[d->video_stream_index]->codec->codec);
                avcodec_copy_context(d->stream->codec,
                                     d->format_ctx->streams[d->video_stream_index]->codec);
                d->stream->sample_aspect_ratio =
                        d->format_ctx->streams[d->video_stream_index]->codec->sample_aspect_ratio;
            }

            int check = 0;
            packet.stream_index = d->stream->id;

//             qInfo() << "decoding";
            int result = avcodec_decode_video2(d->codec_ctx, picture, &check, &packet);
//            qInfo() << "Bytes decoded " << result << " check " << check;


            d->display->DisplayVideoFrame(picture->data, d->codec_ctx->width, d->codec_ctx->height);
            d->cnt++;
        }

//        av_free_packet(&packet);
//        av_init_packet(&packet);
    }

    av_free_packet(&packet);

    av_free(picture);
    av_free(picture_buffer);

    av_read_pause(d->format_ctx);
    avio_close(d->output_ctx->pb);
    avformat_free_context(d->output_ctx);

    emit finished();
}

void Decoder::stop()
{
    Q_D(Decoder);
    d->isStop.store(true);
}

