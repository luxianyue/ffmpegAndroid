package com.lxy.ffmpeg;

import android.graphics.Bitmap;

import java.nio.Buffer;
import java.nio.ByteBuffer;


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
        //System.loadLibrary("avresample");
    }

    public static native String getVersion();

    public static native VFrame getVideoInfo(String path);
    public static native VFrame getVideoFrame(String path);

    public static native Bitmap getVideoImage(String path);

    public static native int exeCmd(String[] cmd);

    public static Bitmap getVideoBitmap(String path) {
        VFrame vFrame = getVideoFrame(path);
        if (vFrame != null) {
            Bitmap bitmap = Bitmap.createBitmap(vFrame.width, vFrame.height, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(ByteBuffer.wrap(vFrame.frame));
            return bitmap;
        }
        return null;
    }

    public static int exeCmd(String cmd) {
        return exeCmd(cmd.split("\\s+"));
    }

}
