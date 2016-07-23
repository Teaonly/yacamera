#include <jni.h>
#include "webrtc/base/basicdefs.h"
#include "webrtc/base/common.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/video_engine/include/vie_base.h"

#include "helper.h"
#include "jnipeer.h"
#include "camerartc.h"

#undef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))
#define JOW(rettype, name) extern "C" rettype JNIEXPORT JNICALL \
      Java_com_yacamera_p2p_NativeAgent_##name

//
//  Global variables
//
CameraRtc* myCamera = NULL;
JniPeer* myPeer = NULL;
JavaVM *myJvm = NULL;
//
//  Internal helper functions
//
static std::string convert_jstring(JNIEnv* env, const jstring &js) {
    static char outbuf[1024*8];
    int len = env->GetStringLength(js);
    env->GetStringUTFRegion(js, 0, len, outbuf);
    std::string str = outbuf;
    return str;
}
static jint get_native_fd(JNIEnv* env, jobject fdesc) {
    jclass clazz;
    jfieldID fid;

    if (!(clazz = env->GetObjectClass(fdesc)) ||
            !(fid = env->GetFieldID(clazz,"descriptor","I"))) return -1;
    return env->GetIntField(fdesc,fid);
}

//
//  Global functions called from Java side 
//
JOW(int, startRtcTask)(JNIEnv* jenv, jclass, jobject context, jobject envfd, jstring cname) {
    std::string cameraName = convert_jstring(jenv, cname);
    int event_fd = (int) get_native_fd(jenv, envfd);
    
    //rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
    //rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
    //rtc::LogMessage::LogTimestamps();
    //rtc::LogMessage::LogThreads();

    //webrtc::VideoEngine::SetAndroidObjects(NULL, context);
    webrtc::VoiceEngine::SetAndroidObjects(myJvm, jenv, context);

    myPeer = new JniPeer(cameraName, event_fd);
    myCamera = new CameraRtc(cameraName, myPeer);

    return 0;
}

JOW(int, stopRtcTask)(JNIEnv*, jclass) {
    if ( myCamera != NULL) {
        //myCamera->Logout();
        delete myCamera;
        myCamera = NULL;
    }
}

JOW(int, pushMessage)(JNIEnv* jenv, jclass, jstring jmsg) {
    std::string msg = convert_jstring(jenv,jmsg);
    myPeer->PushMessage(msg);
    return 0;
}

JOW(int, updatePCM)(JNIEnv* env, jclass, jbyteArray pcmData, jint length) {
    /*
    if ( CameraRecorder::myRecorder == NULL) {
        CameraRecorder::myRecorder = new CameraRecorder();
    } 
    if ( CameraRecorder::myRecorder != NULL) {
        jbyte* audioFrame= env->GetByteArrayElements(pcmData, NULL);
        CameraRecorder::myRecorder->updatePCM((unsigned char *)audioFrame, length);
        env->ReleaseByteArrayElements(pcmData, audioFrame, JNI_ABORT);         
    }
    */
    if ( myCamera != NULL) {
        jbyte* audioFrame= env->GetByteArrayElements(pcmData, NULL);
        myCamera->UpdatePCM((unsigned char *)audioFrame, length);
        env->ReleaseByteArrayElements(pcmData, audioFrame, JNI_ABORT);         
    }
    return 0;
}

JOW(int, updatePicture)(JNIEnv* env, jclass, jbyteArray jpegData, jint length) {
    /*
    if ( CameraCapture::myCapture != NULL) {
        jbyte* cameraFrame= env->GetByteArrayElements(yuvData, NULL);
        CameraCapture::myCapture->updatePicture((unsigned char *)cameraFrame);
        env->ReleaseByteArrayElements(yuvData, cameraFrame, JNI_ABORT);
    }
    */
    if ( myCamera != NULL) {
        jbyte* jpegFrame= env->GetByteArrayElements(jpegData, NULL);
        myCamera->UpdatePicture((unsigned char *)jpegFrame, length); 
        env->ReleaseByteArrayElements(jpegData, jpegFrame, JNI_ABORT);
    }
    return 0;
}

//
//  Top level library load/unload 
//
extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv* jni;
    if (jvm->GetEnv(reinterpret_cast<void**>(&jni), JNI_VERSION_1_6) != JNI_OK)
        return -1;
    rtc::InitializeSSL();
    myJvm = jvm;
    return JNI_VERSION_1_6;
}

extern "C" jint JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM *jvm, void *reserved) {
}

