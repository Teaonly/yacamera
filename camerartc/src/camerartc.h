#ifndef _CAMERA2RTC_H_
#define _CAMERA2RTC_H_

#include <vector>
#include <map>

#include "talk/app/webrtc/datachannelinterface.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/messagequeue.h"
#include "webrtc/base/criticalsection.h"
extern "C" {
#include "g72x/g72x.h"
};

class Peer;
class RtcStream;
class Attendee;
class MediaBuffer;

namespace webrtc {
    class PeerConnectionFactoryInterface;
    class Resampler;
};

class CameraRtc : public sigslot::has_slots<>, public rtc::MessageHandler {
public:
    CameraRtc(const std::string& myName, const std::string& server, const int port);
    CameraRtc(const std::string& myName, Peer *peer);
    ~CameraRtc();

    void Login();
    
    void UpdatePicture(unsigned char *jpegData, int length);
    void UpdatePCM(unsigned char *pcmData, int length); 

protected:
    void init(const std::string& name);

    // call back from peer
    void onOnline(bool isOk);
    void onOffline();
    void onPeerOnline(const std::string&);
    void onPeerOffline(const std::string&);
    void onPeerMessage(const std::string&, const std::vector<std::string>& );
    void onPrintString(const std::string& );

    // message handler in signal thread
    virtual void OnMessage(rtc::Message *msg);
    void streamingTimer_s();
    void peerOnline_s(const std::string& peer);
    void peerOffline_s(const std::string& peer);
    void peerMessage_s(const std::string& peer, const std::vector<std::string>& words);

private:
    // thread resource
    rtc::Thread *signal_thread_;
    std::string my_name_;
    Peer *peer_;
    
    // media resource
    bool          isStreaming_;
    MediaBuffer*  mediaPool_;
    unsigned short  mediaSequence_;
    // audio resource
    g726_state   g726State_;
    webrtc::Resampler* resampler_;
    rtc::CriticalSection adpcmMutex_;
    unsigned char adpcmBuffer_[1024*16];
    unsigned int adpcmLength_;

    // online attendees
    std::map<std::string, Attendee*> onlines_;
};

#endif
