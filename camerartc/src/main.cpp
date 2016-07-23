#include <iostream>
#include <stdlib.h>

#include "webrtc/base/basicdefs.h"
#include "webrtc/base/common.h"
#include "webrtc/base/helpers.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/ssladapter.h"

#include "camerartc.h"

int main(int argc, char *argv[]) {
    rtc::InitializeSSL();
    //rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
    //rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
    //rtc::LogMessage::LogTimestamps();
    //rtc::LogMessage::LogThreads();

    if ( argc < 3) {
        std::cout << "usage: server_ip port " << std::endl;
        return -1;
    }
    
    CameraRtc *rtc = new CameraRtc( "debug", argv[1], atoi(argv[2]) );
    rtc->Login();
    
    rtc::Thread* main_thread = rtc::Thread::Current();
    main_thread->Run();
   
    rtc::CleanupSSL();
    return 0;
}

