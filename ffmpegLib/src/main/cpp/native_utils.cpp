#include <jni.h>
#include <libyuv.h>
#include <libyuv/convert_from.h>
extern "C" {
#include <libavutil/avutil.h>
#include "libavcodec/avcodec.h"
#include "android/log.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include "android/bitmap.h"
#include "libyuv.h"
#include "libyuv/convert_argb.h"
#include "ffmpeg.h"

#define logDebug(...) __android_log_print(ANDROID_LOG_DEBUG,"FFmpegLig",__VA_ARGS__)


JNIEXPORT jstring JNICALL
Java_com_lxy_ffmpeg_FFmpegLib_getVersion(JNIEnv *env, jclass clazz) {
    const char *version = av_version_info();
    return env->NewStringUTF(version);
}

jobject createBitmap(JNIEnv *env, int width, int height) {

    jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
    jmethodID createBitmapId = env->GetStaticMethodID(bitmapCls,"createBitmap",
                                                            "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    jstring configName = env->NewStringUTF("ARGB_8888");
    jclass bitmapConfigClass = env->FindClass("android/graphics/Bitmap$Config");
    jmethodID bitmapConfigId = env->GetStaticMethodID(bitmapConfigClass,"valueOf",
                                                                   "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
    jobject bitmapConfig = env->CallStaticObjectMethod(bitmapConfigClass,bitmapConfigId, configName);
    jobject newBitmap = env->CallStaticObjectMethod(bitmapCls,createBitmapId, width, height, bitmapConfig);
    return newBitmap;
}

JNIEXPORT jobject JNICALL
Java_com_lxy_ffmpeg_FFmpegLib_getVideoInfo(JNIEnv *env, jclass clazz, jstring path) {
    //初始化
    const char *_path = env->GetStringUTFChars(path, JNI_FALSE);
    logDebug("视频路径 == %s", _path);

    AVFormatContext *avFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&avFormatCtx, _path, nullptr, nullptr) != 0) {
        logDebug("解封装失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }
    if (avformat_find_stream_info(avFormatCtx, nullptr) < 0) {
        avformat_close_input(&avFormatCtx);
        logDebug("查找流失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    int video_stream_index = -1;
    for (int i = 0; i < avFormatCtx->nb_streams; i++) {
        if (avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        avformat_close_input(&avFormatCtx);
        logDebug("codecpar没找到 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVCodecParameters *codec_params = avFormatCtx->streams[video_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        avformat_close_input(&avFormatCtx);
        logDebug("没找到解码器 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    jclass frame_class = env->FindClass("com/lxy/ffmpeg/VFrame");
    jobject vFame = env->AllocObject(frame_class);
    jfieldID widthId = env->GetFieldID(frame_class, "width", "I");
    jfieldID heightId = env->GetFieldID(frame_class, "height", "I");
    jfieldID durationId = env->GetFieldID(frame_class, "duration", "J");
    env->SetIntField(vFame, widthId, codec_params->width);
    env->SetIntField(vFame, heightId, codec_params->height);
    env->SetLongField(vFame, durationId, avFormatCtx->duration);

    avformat_close_input(&avFormatCtx);
    env->ReleaseStringUTFChars(path, _path);
    return vFame;
}

JNIEXPORT jobject JNICALL
Java_com_lxy_ffmpeg_FFmpegLib_getVideoFrame(JNIEnv *env, jclass clazz, jstring path) {
    //初始化
    const char *_path = env->GetStringUTFChars(path, JNI_FALSE);
    int ret = -1;
    logDebug("视频路径 == %s", _path);

    AVFormatContext *avFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&avFormatCtx, _path, nullptr, nullptr) != 0) {
        logDebug("解封装失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }
    if (avformat_find_stream_info(avFormatCtx, nullptr) < 0) {
        avformat_close_input(&avFormatCtx);
        logDebug("查找流失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    int video_stream_index = -1;
    for (int i = 0; i < avFormatCtx->nb_streams; i++) {
        if (avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        avformat_close_input(&avFormatCtx);
        logDebug("codecpar没找到 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVCodecParameters *codec_params = avFormatCtx->streams[video_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        avformat_close_input(&avFormatCtx);
        logDebug("没找到解码器 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("codecContext alloc fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("avcodec_open2 fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket packet;
    int response = 0;
    int frame_finished = 0;
    while (av_read_frame(avFormatCtx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            response = avcodec_send_packet(codec_ctx, &packet);
            if (response < 0) {
                break;
            }

            response = avcodec_receive_frame(codec_ctx, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                continue;
            } else if (response < 0) {
                break;
            }

            frame_finished = 1;
            break;
        }
        av_packet_unref(&packet);
    }
    av_packet_unref(&packet);
    if (!frame_finished) {
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("av_read_frame fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    int width = codec_ctx->width;
    int height = codec_ctx->height;
    SwsContext *sws_ctx = sws_getContext(width, height, codec_ctx->pix_fmt,
                                         width, height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    uint8_t *buffer = (uint8_t *) av_malloc(num_bytes * sizeof(uint8_t));
    AVFrame *frame_rgba = av_frame_alloc();
    av_image_fill_arrays(frame_rgba->data, frame_rgba->linesize, buffer, AV_PIX_FMT_RGBA, width, height, 1);
    sws_scale(sws_ctx, (uint8_t const * const *) frame->data, frame->linesize, 0, height, frame_rgba->data, frame_rgba->linesize);

    jbyteArray frame_data = env->NewByteArray(num_bytes);
    env->SetByteArrayRegion(frame_data, 0, num_bytes, (jbyte *) buffer);

    jclass frame_class = env->FindClass("com/lxy/ffmpeg/VFrame");
    jobject vFame = env->AllocObject(frame_class);
    jfieldID durationId = env->GetFieldID(frame_class, "duration", "J");
    env->SetIntField(vFame, env->GetFieldID(frame_class, "width", "I"), width);
    env->SetIntField(vFame, env->GetFieldID(frame_class, "height", "I"), height);
    env->SetLongField(vFame, durationId, avFormatCtx->duration);
    env->SetObjectField(vFame, env->GetFieldID(frame_class, "frame", "[B"), frame_data);

    av_free(buffer);
    av_frame_free(&frame_rgba);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&avFormatCtx);
    env->ReleaseStringUTFChars(path, _path);
    return vFame;
}


JNIEXPORT jobject JNICALL
Java_com_lxy_ffmpeg_FFmpegLib_getVideoImage(JNIEnv *env, jclass clazz, jstring path) {
    const char *_path = env->GetStringUTFChars(path, JNI_FALSE);
    logDebug("视频路径 == %s", _path);

    AVFormatContext *avFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&avFormatCtx, _path, nullptr, nullptr) != 0) {
        logDebug("解封装失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }
    if (avformat_find_stream_info(avFormatCtx, nullptr) < 0) {
        avformat_close_input(&avFormatCtx);
        logDebug("查找流失败 -- %s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    int video_stream_index = -1;
    for (int i = 0; i < avFormatCtx->nb_streams; i++) {
        if (avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        avformat_close_input(&avFormatCtx);
        logDebug("codecpar没找到 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVCodecParameters *codec_params = avFormatCtx->streams[video_stream_index]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        avformat_close_input(&avFormatCtx);
        logDebug("没找到解码器 --%s", _path);
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("codecContext alloc fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("avcodec_open2 fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVFrame *frame = av_frame_alloc();
    AVPacket packet;
    int response = 0;
    int frame_finished = 0;
    while (av_read_frame(avFormatCtx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            response = avcodec_send_packet(codec_ctx, &packet);
            if (response < 0) {
                break;
            }

            response = avcodec_receive_frame(codec_ctx, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                continue;
            } else if (response < 0) {
                break;
            }

            frame_finished = 1;
            break;
        }
        av_packet_unref(&packet);
    }
    av_packet_unref(&packet);
    if (!frame_finished) {
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("av_read_frame fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    SwsContext *sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                         codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_ctx) {
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("sws_getContext fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    AVFrame *rgb_frame = av_frame_alloc();
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, codec_ctx->width, codec_ctx->height, 1);
    uint8_t *buffer = (uint8_t *) av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_RGBA, codec_ctx->width, codec_ctx->height, 1);

    sws_scale(sws_ctx, (uint8_t const *const *) frame->data, frame->linesize, 0, codec_ctx->height, rgb_frame->data, rgb_frame->linesize);
    jobject bitmap = createBitmap(env, codec_ctx->width, codec_ctx->height);

    void *bitmap_pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &bitmap_pixels) < 0) {
        av_free(buffer);
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&avFormatCtx);
        logDebug("AndroidBitmap_lockPixels fail");
        env->ReleaseStringUTFChars(path, _path);
        return nullptr;
    }

    memcpy(bitmap_pixels, rgb_frame->data[0], num_bytes);
    AndroidBitmap_unlockPixels(env, bitmap);

    av_free(buffer);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&avFormatCtx);
    env->ReleaseStringUTFChars(path, _path);
    return bitmap;



    //初始化
    /*const char *_path = env->GetStringUTFChars(path, JNI_FALSE);
    int ret = -1;
    logDebug("视频路径 == %s", _path);

    //代码逻辑：
    // 解封装 ，找到编解码器，解码，获取yuv，转rgb，封装bitmap返回
    //封装格式上下文
    AVFormatContext *ifmt_ctx = nullptr;
    ret = avformat_open_input(&ifmt_ctx, _path, nullptr, nullptr);
    if (ret < 0) {
        logDebug("解封装失败 -- %s", av_err2str(ret));
        return nullptr;
    }
    ret = avformat_find_stream_info(ifmt_ctx, nullptr);
    if (ret < 0) {
        logDebug("查找流失败");
        return nullptr;
    }

//    av_dump_format(ifmt_ctx, 0, _path, 0);
    //定义视频流相关的参数
    int video_stream_index = -1;
    AVStream *pStream = nullptr;
    AVCodecParameters *codecpar = nullptr;
    //找出视频流
    for (int i = 0; i < ifmt_ctx->nb_streams; ++i) {
        pStream = ifmt_ctx->streams[i];
        if (pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            codecpar = pStream->codecpar;
            video_stream_index = i;
        }
    }
    if (!codecpar) {
        logDebug("codecpar没找到");
        return nullptr;
    }

    //找到最合适的流
    int stream_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    logDebug("video_stream_index == %d", video_stream_index);
    logDebug("stream_index == %d", stream_index);

    logDebug("解码器 == %s", avcodec_get_name(codecpar->codec_id));
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        logDebug("没找到解码器");
        return nullptr;
    }
    //申请编解码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        logDebug("codecContext alloc error");
        return nullptr;
    }
    //拷贝parameters 到 context
    ret = avcodec_parameters_to_context(codec_ctx, codecpar);
    if (ret < 0) {
        logDebug("avcodec_parameters_to_context  error");
        return nullptr;
    }
    logDebug("视频帧像素格式---> %s", av_get_pix_fmt_name(codec_ctx->pix_fmt));

    //打开编解码器
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        logDebug("打开解码器失败 -- %s", av_err2str(ret));
        return nullptr;
    }

    int width = codec_ctx->width;
    int height = codec_ctx->height;

    logDebug("codecContext->width == %d", width);
    logDebug("codecContext->height == %d", height);

    jobject bmp = createBitmap(env, codec_ctx->width, codec_ctx->height);
    if (!bmp) {
        logDebug("bmp == null");
    }
    void *addr_pixels;
    ret = AndroidBitmap_lockPixels(env, bmp, &addr_pixels);
    if (ret < 0) {
        logDebug("lockPixel error");
        return nullptr;
    }
    AndroidBitmapInfo info;
    ret = AndroidBitmap_getInfo(env, bmp, &info);
    if (ret < 0) {
        logDebug("getInfo error");
        return nullptr;
    }

    bool finish = false;
    //申请一个帧结构体
    AVPacket pkg;
    AVFrame *pFrame = av_frame_alloc();
    while (av_read_frame(ifmt_ctx, &pkg) >= 0) {
        if (pkg.stream_index != video_stream_index) {
            continue;
        }
        logDebug("av_read_frame");
        //Use avcodec_send_packet() and avcodec_receive_frame().
        ret = avcodec_send_packet(codec_ctx, &pkg);
        if (ret < 0) {
            logDebug("Error sending packet for decoding");
            return nullptr;
        }
        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_ctx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            finish = true;
            if (ret < 0) {
                logDebug("Error during decoding");
                return nullptr;
            }
            break;
        }
        if (finish) {
            logDebug("读取首帧完毕");
            break;
        }
    }

    //yuv420p to argb
    int linesize = pFrame->width * 4;
    libyuv::I420ToABGR(pFrame->data[0], pFrame->linesize[0], // Y
                       pFrame->data[1], pFrame->linesize[1], // U
                       pFrame->data[2], pFrame->linesize[2], // V
                       (uint8_t *) addr_pixels, linesize,  // RGBA
                       pFrame->width, pFrame->height);

    //释放资源
    logDebug("开始释放资源");
    AndroidBitmap_unlockPixels(env, bmp);
    av_packet_unref(&pkg);
    av_free(pFrame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&ifmt_ctx);
    env->ReleaseStringUTFChars(path, _path);
    return bmp;*/
}

JNIEXPORT jint JNICALL
Java_com_lxy_ffmpeg_FFmpegLib_exeCmd(JNIEnv *env, jclass clazz, jobjectArray cmd) {
    int argc = env->GetArrayLength(cmd);
    logDebug("argc == %d", argc);
    char *argv[argc];

    for (int i = 0; i < argc; ++i) {
        jstring str = (jstring) env->GetObjectArrayElement(cmd, i);
        const char *cstr = env->GetStringUTFChars(str, JNI_FALSE);
        argv[i] = new char[strlen(cstr)];
        strcpy(argv[i], cstr);
        env->ReleaseStringUTFChars(str, cstr);
        logDebug("%s ", argv[i]);
    }

    int ref = exe_cmd(argc, argv);
    delete *argv;
    return ref;
    //return 1;
}


}