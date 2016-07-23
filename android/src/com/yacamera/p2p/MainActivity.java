package com.yacamera.p2p;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.locks.ReentrantLock;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.apache.http.conn.util.InetAddressUtils;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.PictureCallback;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.Paint;
import android.graphics.YuvImage;
import android.media.AudioFormat;
import android.media.MediaRecorder;
import android.media.AudioRecord;
import android.os.Bundle;
import android.os.Looper;
import android.os.Handler;
import android.util.*;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.google.android.gms.ads.*;

public class MainActivity extends Activity
        implements CameraView.CameraReadyCallback, NativeAgent.ConnectCallback {
    public static String TAG="TEAONLY";
    private AdView adView;

    private final String serverAddress = "ws://www.yacamera.com/ws";
    //private final String serverAddress = "ws://10.3.4.13:1982/ws";
    //private final String serverAddress = "ws://192.168.1.100:1982/ws";

    private String cameraName = UUID.randomUUID().toString().substring(0, 5);
    //private String cameraName = "debug";

    private OverlayView overlayView = null;

    private NativeAgent nativeAgent;

    final int pictureRate = 6;
    int     pictureSeq = 1;

    ExecutorService executor = Executors.newFixedThreadPool(3);
    private ReentrantLock previewLock = new ReentrantLock();

    MediaBuffer jpegPicture = new MediaBuffer(1024*1024*2);
    byte[] yuvFrame = new byte[1920*1280*2];
    byte[] avcNals = new byte[1024*1024*4];
    int avcLength = 0;
    int avcSeq = 0;
    final int avcSeqReset = 4;
    boolean inProcessing = false;

    private CameraView cameraView = null;
    private AudioRecord audioCapture = null;

    //
    //  Activiity's event handler
    //
    @Override
    public void onCreate(Bundle savedInstanceState) {
        // application setting
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        Window win = getWindow();
        win.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // load and setup GUI
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        /*
        AdView adView = (AdView)this.findViewById(R.id.adView);
        AdRequest adRequest = new AdRequest.Builder().build();
        adView.loadAd(adRequest);
        */
        adView = new AdView(this);
        adView.setAdUnitId("ca-app-pub-7979468066645196/8494952268");
        adView.setAdSize(AdSize.BANNER);
        LinearLayout layout = (LinearLayout)findViewById(R.id.layout_ad);
        layout.addView(adView);
        AdRequest adRequest = new AdRequest.Builder().build();
        adView.loadAd(adRequest);
 
        nativeAgent = new NativeAgent(this);

        // init audio and camera
        initAudio();
        initCamera();
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onPause() {
        super.onPause();

        if ( cameraView != null) {
            previewLock.lock();
            cameraView.StopPreview();
            cameraView.Release();
            previewLock.unlock();
            cameraView = null;
        }
                
        nativeAgent.stopNative();

        finish();
        //System.exit(0);
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }

    //
    //  Interface implementation
    //
    public void onCameraReady() {
        nativeAgent.startNative(this, serverAddress, cameraName);

        cameraView.StopPreview();
        cameraView.setupCamera(320, 240, 4, 25.0, previewCb);
        //Log.d(TAG, ">>>Camera size = " + cameraView.Width() + " " + cameraView.Height());
        nativeAgent.initVideoEncoder(cameraView.Width(), cameraView.Height() );
        cameraView.StartPreview();
    }

    public void onConnected() {
        new Handler(Looper.getMainLooper()).post( new Runnable() {
            @Override
            public void run() {
                TextView tv = (TextView)findViewById(R.id.tv_message);
                tv.setText("Open http://yacamera.com, input camera code: " + cameraName);
            }
        });
    }

    public void onDisconnected() {
        if ( cameraView == null) {
            return;
        }

        new Handler(Looper.getMainLooper()).post( new Runnable() {
            @Override
            public void run() {
                TextView tv = (TextView)findViewById(R.id.tv_message);
                tv.setText("Disconncted from server, will retry after 2 seconds.");
                nativeAgent.stopNative();

                new Handler(Looper.getMainLooper()).postDelayed( new Runnable() {
                    @Override
                    public void run() {
                        TextView tv = (TextView)findViewById(R.id.tv_message);
                        tv.setText("Reconnecting...");
                        nativeAgent.startNative(MainActivity.this, serverAddress, cameraName);
                    }
                }, 2000);
            }
        });
    }

    public void onLoginError() {
        // change the camera name
        cameraName = UUID.randomUUID().toString().substring(0, 5);
    }

    //
    //  Internal help functions
    //
    private void initCamera() {
        SurfaceView cameraSurface = (SurfaceView)findViewById(R.id.surface_camera);
        cameraView = new CameraView(cameraSurface);
        cameraView.setCameraReadyCallback(this);

        overlayView = (OverlayView)findViewById(R.id.surface_overlay);
        //overlayView_.setOnTouchListener(this);
        //overlayView_.setUpdateDoneCallback(this);
    }
    private void initAudio() {
        int minBufferSize = AudioRecord.getMinBufferSize(16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
        int targetSize = 16000 * 2;      // 1 seconds buffer size
        if (targetSize < minBufferSize) {
            targetSize = minBufferSize;
        }
        if (audioCapture == null) {
            audioCapture = new AudioRecord(MediaRecorder.AudioSource.MIC,
                    16000,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT,
                    targetSize);
        }

        audioCapture.startRecording();
        AudioEncoder audioEncoder = new AudioEncoder();
        audioEncoder.start();
    }


    //
    //  Internal help class and object definment
    //
    private PreviewCallback previewCb = new PreviewCallback() {
        public void onPreviewFrame(byte[] frame, Camera c) {
            {
                previewLock.lock();

                pictureSeq ++;
                if ( (pictureSeq % pictureRate) == 0) {
                    if ( false ) {
                        int picWidth = cameraView.Width();
                        int picHeight = cameraView.Height();

                        YuvImage newImage = new YuvImage(frame, ImageFormat.NV21, picWidth, picHeight, null);
                        jpegPicture.reset();
                        boolean ret = false;
                        try {
                            ret = newImage.compressToJpeg( new Rect(0,0,picWidth,picHeight), 30, jpegPicture);
                        } catch (Exception ex) {

                        }

                        if ( ret ) {
                            nativeAgent.updatePicture(jpegPicture.data(), jpegPicture.length());
                        }
                    } else {
                        doVideoEncode(frame);
                    }
                }

                previewLock.unlock();
            }

            c.addCallbackBuffer(frame);
        }
    };

    private void doVideoEncode(byte[] frame) {
        synchronized(this) {
            if ( inProcessing == true) {
                return;
            }
            inProcessing = true;
        }

        int picWidth = cameraView.Width();
        int picHeight = cameraView.Height();
        int size = picWidth*picHeight + picWidth*picHeight/2;
        System.arraycopy(frame, 0, yuvFrame, 0, size);

        VideoEncodingTask videoTask = new  VideoEncodingTask();
        executor.execute(videoTask);
    };

    private class VideoEncodingTask implements Runnable {
        private byte[] resultNal = new byte[1024*1024];

        public void run() {
            int ret = 0;
            if ( avcSeq == 0) {
                ret = nativeAgent.doVideoEncode(yuvFrame, resultNal, 1);
            } else {
                ret = nativeAgent.doVideoEncode(yuvFrame, resultNal, 0);
            }

            if ( ret > 0) {
                
                System.arraycopy(resultNal, 0, avcNals, avcLength, ret);
                avcLength += ret;

                avcSeq ++;
                if ( avcSeq == avcSeqReset ) {
                    avcSeq = 0;
                    //Log.d(TAG, ">>>>> Video Size = " + avcLength + " data:" + avcNals[0] + avcNals[1] + avcNals[3]);
                    nativeAgent.updatePicture(avcNals, avcLength);
                    avcLength = 0;
                }
            }

            synchronized(MainActivity.this) {
                inProcessing = false;
            }
        }
    };

    private class AudioEncoder extends Thread {
        byte[] audioPackage = new byte[1024*32];
        int packageSize = 8000*2;   // 0.5 seconds

        @Override
        public void run() {
            while(true) {
                int ret = audioCapture.read(audioPackage, 0, packageSize);
                if ( ret == AudioRecord.ERROR_INVALID_OPERATION ||
                     ret == AudioRecord.ERROR_BAD_VALUE) {
                    break;
                }
                nativeAgent.updatePCM(audioPackage, ret);
            }
        }
    }

}
