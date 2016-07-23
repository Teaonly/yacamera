package com.yacamera.p2p;

import java.io.*;
import java.net.*;
import java.util.*;

import android.content.Context;
import android.net.*;
import android.util.Log;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;

public class NativeAgent {
    private static String TAG="TEAONLY";
    private LocalSocket jSock, nSock;
    private LocalServerSocket lss;
    private WebSocketClient wc = null;
    private ConnectCallback loginCb;

    private native int stopRtcTask();
    private native int startRtcTask(Context c, FileDescriptor fd, String cname);
    private native int pushMessage(String message);
    public  native int updatePicture(byte[] data, int length);
    public  native int updatePCM(byte[] data, int length);

    public  native void initVideoEncoder(int wid, int hei);
    public  native void releaseVideoEncoder();
    public  native int doVideoEncode(byte[] in, byte[] out, int flag);

    public static interface ConnectCallback {
        public void onConnected();
        public void onDisconnected();
        public void onLoginError();
    }

    public NativeAgent(ConnectCallback cb) {
        loginCb = cb;

        Random rnd = new Random();
        String localAddress = "com.yacamera.p2p_" + Integer.toHexString(rnd.nextInt());

        try {
            jSock = new LocalSocket();

            lss = new LocalServerSocket(localAddress);
            jSock.connect(new LocalSocketAddress(localAddress));
            jSock.setReceiveBufferSize(1000);
            jSock.setSendBufferSize(1000);

            nSock = lss.accept();
            nSock.setReceiveBufferSize(1000);
            nSock.setSendBufferSize(1000);

        } catch ( IOException ex) {
            ex.printStackTrace();
        }


        // setup websocket , see https://github.com/TooTallNate/Java-WebSocket
        java.lang.System.setProperty("java.net.preferIPv6Addresses", "false");
        java.lang.System.setProperty("java.net.preferIPv4Stack", "true");

        loopThread.start();
    }

    public void startNative(Context c, String serverAddres, String cname) {
        startRtcTask(c, nSock.getFileDescriptor(), cname);
        doConnect(serverAddres, cname);
    }

    public void stopNative() {
        if ( wc != null) {
            wc.close();
            wc = null;
        }
        stopRtcTask( );
    }

    private void doConnect(String serverAddres, String cameraName) {
        URI wsServer = null;
        try {
            wsServer = new URI(serverAddres + "/camera_" + cameraName);
        } catch ( URISyntaxException e) {
            loginCb.onDisconnected();
            return;
        }

        wc = new WebSocketClient( wsServer) {
            @Override
            public void onMessage( String message ) {
                if ( message == "<login:error>" ) {
                    loginCb.onLoginError();
                }
                pushMessage(message);
            }

            @Override
            public void onOpen( ServerHandshake handshakedata ) {
                loginCb.onConnected();
            }

            @Override
            public void onClose( int code, String reason, boolean remote ) {
                loginCb.onDisconnected();
            }

            @Override
            public void onError( Exception ex ) {
                //loginCb.onDisconnected();
            }
        };

        wc.connect();
    }

    private Thread loopThread = new Thread() {
        @Override
        public void run() {
            byte[] receiveData = new byte[1024*16];
            while(true) {
                try {
                    int ret = jSock.getInputStream().read(receiveData);
                    if ( ret < 0)
                        break;
                    if ( ret == 0)
                        continue;

                    String xmlBuffer = new String(receiveData, 0, ret);
                    if ( wc != null) {
                        wc.send(xmlBuffer);
                    }
                } catch (IOException ex) {
                    ex.printStackTrace();
                    break;
                }
            }
        }
    };

    static {
        System.loadLibrary("jingle_camerartc_so");
        System.loadLibrary("x264encoder");
    }
}
