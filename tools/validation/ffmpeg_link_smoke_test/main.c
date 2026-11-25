/*
 * Isolated dynamic-link smoke test for the approved FFmpeg set.
 * Not product code. Does not link libavfilter or libavdevice.
 *
 * With no argument, writes a synthetic NUT (one video frame plus audio),
 * reopens it, inspects streams, decodes one video frame and the audio,
 * converts one frame with libswscale and one buffer with libswresample,
 * frees every allocation, and deletes the synthetic file.
 *
 * A path argument opens that file instead and does not delete it.
 * Any failed step prints the reason and returns non-zero.
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/version.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_one(const char *name, unsigned version)
{
    printf("%s runtime %u.%u.%u (0x%08x)\n",
           name,
           AV_VERSION_MAJOR(version),
           AV_VERSION_MINOR(version),
           AV_VERSION_MICRO(version),
           version);
}

static void print_versions(void)
{
    print_one("libavutil", avutil_version());
    print_one("libavcodec", avcodec_version());
    print_one("libavformat", avformat_version());
    print_one("libswscale", swscale_version());
    print_one("libswresample", swresample_version());
    printf("libavutil compile-time ident: %s\n", av_version_info());
}

static int fail(const char *step, int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    if (err < 0) {
        av_strerror(err, buf, sizeof buf);
        fprintf(stderr, "FAIL %s: %s (%d)\n", step, buf, err);
    } else {
        fprintf(stderr, "FAIL %s\n", step);
    }
    return 1;
}

static int write_synthetic(const char *path)
{
    AVFormatContext *oc = NULL;
    AVCodecContext *venc = NULL;
    AVCodecContext *aenc = NULL;
    AVStream *vst = NULL;
    AVStream *ast = NULL;
    AVFrame *vframe = NULL;
    AVFrame *aframe = NULL;
    AVPacket *pkt = NULL;
    const AVCodec *vcodec;
    const AVCodec *acodec;
    int ret = 0;
    int failed = 0;

    vcodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    acodec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    if (!vcodec || !acodec) {
        fprintf(stderr, "FAIL synthetic: mjpeg or pcm_s16le encoder not found\n");
        return 1;
    }
    ret = avformat_alloc_output_context2(&oc, NULL, "nut", path);
    if (ret < 0 || !oc) {
        return fail("alloc output", ret < 0 ? ret : AVERROR_UNKNOWN);
    }
    vst = avformat_new_stream(oc, NULL);
    ast = avformat_new_stream(oc, NULL);
    if (!vst || !ast) {
        failed = fail("new stream", AVERROR(ENOMEM));
        goto done;
    }
    venc = avcodec_alloc_context3(vcodec);
    aenc = avcodec_alloc_context3(acodec);
    if (!venc || !aenc) {
        failed = fail("alloc encoder", AVERROR(ENOMEM));
        goto done;
    }
    venc->width = 16;
    venc->height = 16;
    venc->pix_fmt = AV_PIX_FMT_YUVJ420P;
    venc->time_base = (AVRational){1, 24};
    aenc->sample_fmt = AV_SAMPLE_FMT_S16;
    aenc->sample_rate = 48000;
    av_channel_layout_default(&aenc->ch_layout, 2);
    aenc->time_base = (AVRational){1, 48000};
    if ((ret = avcodec_open2(venc, vcodec, NULL)) < 0 ||
        (ret = avcodec_open2(aenc, acodec, NULL)) < 0) {
        failed = fail("open encoder", ret);
        goto done;
    }
    if ((ret = avcodec_parameters_from_context(vst->codecpar, venc)) < 0 ||
        (ret = avcodec_parameters_from_context(ast->codecpar, aenc)) < 0) {
        failed = fail("parameters from context", ret);
        goto done;
    }
    vst->time_base = venc->time_base;
    ast->time_base = aenc->time_base;
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&oc->pb, path, AVIO_FLAG_WRITE)) < 0) {
            failed = fail("avio_open", ret);
            goto done;
        }
    }
    if ((ret = avformat_write_header(oc, NULL)) < 0) {
        failed = fail("write header", ret);
        goto done;
    }
    vframe = av_frame_alloc();
    aframe = av_frame_alloc();
    pkt = av_packet_alloc();
    if (!vframe || !aframe || !pkt) {
        failed = fail("alloc frames", AVERROR(ENOMEM));
        goto done;
    }
    vframe->format = venc->pix_fmt;
    vframe->width = venc->width;
    vframe->height = venc->height;
    if ((ret = av_frame_get_buffer(vframe, 0)) < 0) {
        failed = fail("video buffer", ret);
        goto done;
    }
    memset(vframe->data[0], 16, (size_t)vframe->linesize[0] * 16);
    vframe->pts = 0;
    aframe->format = AV_SAMPLE_FMT_S16;
    aframe->sample_rate = 48000;
    aframe->nb_samples = 64;
    if ((ret = av_channel_layout_copy(&aframe->ch_layout, &aenc->ch_layout)) < 0 ||
        (ret = av_frame_get_buffer(aframe, 0)) < 0) {
        failed = fail("audio buffer", ret);
        goto done;
    }
    memset(aframe->data[0], 0, (size_t)aframe->nb_samples * 4);
    aframe->pts = 0;
    if ((ret = avcodec_send_frame(venc, vframe)) < 0 ||
        (ret = avcodec_receive_packet(venc, pkt)) < 0) {
        failed = fail("encode video", ret);
        goto done;
    }
    av_packet_rescale_ts(pkt, venc->time_base, vst->time_base);
    pkt->stream_index = vst->index;
    if ((ret = av_interleaved_write_frame(oc, pkt)) < 0) {
        failed = fail("write video", ret);
        goto done;
    }
    av_packet_unref(pkt);
    if ((ret = avcodec_send_frame(aenc, aframe)) < 0 ||
        (ret = avcodec_receive_packet(aenc, pkt)) < 0) {
        failed = fail("encode audio", ret);
        goto done;
    }
    av_packet_rescale_ts(pkt, aenc->time_base, ast->time_base);
    pkt->stream_index = ast->index;
    if ((ret = av_interleaved_write_frame(oc, pkt)) < 0) {
        failed = fail("write audio", ret);
        goto done;
    }
    if ((ret = av_write_trailer(oc)) < 0) {
        failed = fail("trailer", ret);
        goto done;
    }
    printf("synthetic file written: %s\n", path);

done:
    av_packet_free(&pkt);
    av_frame_free(&vframe);
    av_frame_free(&aframe);
    avcodec_free_context(&venc);
    avcodec_free_context(&aenc);
    if (oc) {
        if (!(oc->oformat->flags & AVFMT_NOFILE) && oc->pb)
            avio_closep(&oc->pb);
        avformat_free_context(oc);
    }
    return failed;
}

static int exercise_sws(const AVFrame *src)
{
    struct SwsContext *sws;
    uint8_t *dst_data[4] = {0};
    int dst_linesize[4] = {0};
    int ret;
    sws = sws_getContext(src->width, src->height, (enum AVPixelFormat)src->format,
                         src->width, src->height, AV_PIX_FMT_RGB24,
                         SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws)
        return fail("sws_getContext", AVERROR(EINVAL));
    ret = av_image_alloc(dst_data, dst_linesize, src->width, src->height, AV_PIX_FMT_RGB24, 1);
    if (ret < 0) {
        sws_freeContext(sws);
        return fail("rgb alloc", ret);
    }
    ret = sws_scale(sws, (const uint8_t *const *)src->data, src->linesize, 0, src->height,
                    dst_data, dst_linesize);
    sws_freeContext(sws);
    av_freep(&dst_data[0]);
    if (ret <= 0)
        return fail("sws_scale", AVERROR(EINVAL));
    printf("swscale converted one frame to rgb24, rows=%d\n", ret);
    return 0;
}

static int exercise_swr(const AVFrame *src)
{
    SwrContext *swr = NULL;
    AVChannelLayout out_layout;
    uint8_t *out_data = NULL;
    int out_linesize = 0;
    int out_samples;
    int converted;
    int ret;
    av_channel_layout_default(&out_layout, 2);
    ret = swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_FLT, src->sample_rate,
                              &src->ch_layout, (enum AVSampleFormat)src->format,
                              src->sample_rate, 0, NULL);
    av_channel_layout_uninit(&out_layout);
    if (ret < 0 || !swr)
        return fail("swr_alloc_set_opts2", ret < 0 ? ret : AVERROR(EINVAL));
    if ((ret = swr_init(swr)) < 0) {
        swr_free(&swr);
        return fail("swr_init", ret);
    }
    out_samples = swr_get_out_samples(swr, src->nb_samples);
    ret = av_samples_alloc(&out_data, &out_linesize, 2, out_samples, AV_SAMPLE_FMT_FLT, 0);
    if (ret < 0) {
        swr_free(&swr);
        return fail("swr buffer", ret);
    }
    converted = swr_convert(swr, &out_data, out_samples,
                            (const uint8_t **)src->data, src->nb_samples);
    av_freep(&out_data);
    swr_free(&swr);
    if (converted < 0)
        return fail("swr_convert", converted);
    printf("swresample converted %d audio samples to float stereo\n", converted);
    return 0;
}

static int inspect_and_decode(const char *path)
{
    AVFormatContext *fmt = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    AVCodecContext *vc = NULL;
    AVCodecContext *ac = NULL;
    int video_index = -1;
    int audio_index = -1;
    int got_video = 0;
    int got_audio = 0;
    int ret;
    int failed = 0;
    unsigned i;

    if ((ret = avformat_open_input(&fmt, path, NULL, NULL)) < 0)
        return fail("open input", ret);
    if ((ret = avformat_find_stream_info(fmt, NULL)) < 0) {
        failed = fail("find stream info", ret);
        goto done;
    }
    printf("streams=%u path=%s\n", fmt->nb_streams, path);
    for (i = 0; i < fmt->nb_streams; i++) {
        AVStream *st = fmt->streams[i];
        printf("stream %u type=%d codec=%s time_base=%d/%d\n",
               i, st->codecpar->codec_type,
               avcodec_get_name(st->codecpar->codec_id),
               st->time_base.num, st->time_base.den);
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_index < 0)
            video_index = (int)i;
        if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < 0)
            audio_index = (int)i;
    }
    if (video_index >= 0) {
        const AVCodec *codec = avcodec_find_decoder(fmt->streams[video_index]->codecpar->codec_id);
        if (!codec) {
            failed = fail("video decoder not found", AVERROR_DECODER_NOT_FOUND);
            goto done;
        }
        vc = avcodec_alloc_context3(codec);
        if (!vc ||
            (ret = avcodec_parameters_to_context(vc, fmt->streams[video_index]->codecpar)) < 0 ||
            (ret = avcodec_open2(vc, codec, NULL)) < 0) {
            failed = fail("open video decoder", ret < 0 ? ret : AVERROR(ENOMEM));
            goto done;
        }
    } else {
        printf("no video stream in this file\n");
    }
    if (audio_index >= 0) {
        const AVCodec *codec = avcodec_find_decoder(fmt->streams[audio_index]->codecpar->codec_id);
        if (!codec) {
            failed = fail("audio decoder not found", AVERROR_DECODER_NOT_FOUND);
            goto done;
        }
        ac = avcodec_alloc_context3(codec);
        if (!ac ||
            (ret = avcodec_parameters_to_context(ac, fmt->streams[audio_index]->codecpar)) < 0 ||
            (ret = avcodec_open2(ac, codec, NULL)) < 0) {
            failed = fail("open audio decoder", ret < 0 ? ret : AVERROR(ENOMEM));
            goto done;
        }
    } else {
        printf("no audio stream in this file\n");
    }
    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    if (!pkt || !frame) {
        failed = fail("alloc packet", AVERROR(ENOMEM));
        goto done;
    }
    while ((!got_video && video_index >= 0) || (!got_audio && audio_index >= 0)) {
        ret = av_read_frame(fmt, pkt);
        if (ret < 0)
            break;
        if (pkt->stream_index == video_index && vc && !got_video) {
            if ((ret = avcodec_send_packet(vc, pkt)) < 0) {
                failed = fail("send video", ret);
                av_packet_unref(pkt);
                goto done;
            }
            ret = avcodec_receive_frame(vc, frame);
            if (ret == 0) {
                got_video = 1;
                printf("decoded video frame %dx%d format=%d\n",
                       frame->width, frame->height, frame->format);
                if (exercise_sws(frame))
                    failed = 1;
                av_frame_unref(frame);
            } else if (ret != AVERROR(EAGAIN)) {
                failed = fail("receive video", ret);
                av_packet_unref(pkt);
                goto done;
            }
        } else if (pkt->stream_index == audio_index && ac && !got_audio) {
            if ((ret = avcodec_send_packet(ac, pkt)) < 0) {
                failed = fail("send audio", ret);
                av_packet_unref(pkt);
                goto done;
            }
            ret = avcodec_receive_frame(ac, frame);
            if (ret == 0) {
                got_audio = 1;
                printf("decoded audio samples=%d rate=%d format=%d\n",
                       frame->nb_samples, frame->sample_rate, frame->format);
                if (exercise_swr(frame))
                    failed = 1;
                av_frame_unref(frame);
            } else if (ret != AVERROR(EAGAIN)) {
                failed = fail("receive audio", ret);
                av_packet_unref(pkt);
                goto done;
            }
        }
        av_packet_unref(pkt);
        if (failed)
            goto done;
    }
    if (video_index >= 0 && !got_video) {
        failed = fail("no video frame decoded", AVERROR_INVALIDDATA);
        goto done;
    }
    if (audio_index >= 0 && !got_audio) {
        failed = fail("no audio decoded", AVERROR_INVALIDDATA);
        goto done;
    }
    printf("inspect complete\n");

done:
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&vc);
    avcodec_free_context(&ac);
    avformat_close_input(&fmt);
    return failed;
}

int main(int argc, char **argv)
{
    char path[] = "/tmp/aether-ffmpeg-link-smoke.nut";
    int own_file = 0;
    int failed;
    const char *open_path;

    print_versions();
    if (argc > 1) {
        open_path = argv[1];
    } else {
        own_file = 1;
        if (write_synthetic(path))
            return 1;
        open_path = path;
    }
    failed = inspect_and_decode(open_path);
    if (own_file)
        remove(path);
    if (failed)
        printf("RESULT failed\n");
    else
        printf("RESULT passed\n");
    return failed ? 1 : 0;
}
