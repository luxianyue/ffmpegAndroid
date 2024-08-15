package com.lxy.ffmpeg;

import android.graphics.Bitmap;


public class FFmpegLib {

    private static final String TAG = "FFmpegLib";
    //

    static {
        System.loadLibrary("my_native");
        System.loadLibrary("avcodec");
        System.loadLibrary("avdevice");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
    }

    public static native String getVersion();

    public static native Bitmap getVideoImage(String path);

    public static native int exeCmd(String[] cmd);

    public static int exeCmd(String cmd) {
        return exeCmd(cmd.split("\\s+"));
    }

}
