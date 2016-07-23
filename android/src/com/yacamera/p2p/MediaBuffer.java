package com.yacamera.p2p;

import android.util.Log;
import java.io.*;

class MediaBuffer extends OutputStream {
    private byte[] buffer;
    private int bufferLength;
    private int currentLength;

    public MediaBuffer (int maxSize) {
        super();
        buffer = new byte[maxSize];
        bufferLength = maxSize;
        currentLength = 0;
    }

    public void reset() {
        currentLength = 0;
    }

    public int length() {
        return currentLength;
    }

    public byte[] data() {
        return buffer;
    }

    @Override
    public void write(int oneByte) throws IOException{
        if ( currentLength >= bufferLength) {
            IOException ex = new IOException("Buffer overflow");
            throw ex;
        }
        buffer[currentLength] = (byte)(oneByte & 0xFF);
        currentLength++;
    }

    @Override
    public void close() throws IOException {
        super.close();
    }
}
