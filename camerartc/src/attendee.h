#ifndef _ATTENDEE_H_
#define _ATTENDEE_H_

#include <string>
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/messagequeue.h"
#include "talk/app/webrtc/datachannelinterface.h"

class Peer;
class RtcStream;

namespace webrtc {
    class PeerConnectionFactoryInterface;
}

class Attendee : public sigslot::has_slots<>,
                 public rtc::MessageHandler {
public:
    Attendee(const std::string& viewerName,
             Peer* peer);
    ~Attendee();

    inline std::string name() {
        return attendeeName_;
    }

    int  sendMediaPackage(unsigned char *payload, unsigned int length);
    void processMessage(const std::vector<std::string>& words);

protected:
    virtual void OnMessage(rtc::Message *msg);

private:
    // callback from RTC
    void onLocalDescription(RtcStream* stream, const std::string& desc);
    void onLocalCandidate(RtcStream* stream, const std::string& cand);

    // callback from DataChannel
    void onChannelData(RtcStream*, const webrtc::DataBuffer& buffer);
    void onChannelOpened(RtcStream*, const std::string& cname);
    void onChannelClosed(RtcStream*);

    // help functions
    void sendMessage(const std::string& v0, const std::string& v1);
    void sendMessage(const std::string& v0, const std::string& v1, const std::string& v2);

private:
    // Self information
    std::string attendeeName_;

    // Application stuff
    Peer* peer_;

    // RTC stuff
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
    RtcStream* stream_;
    bool rtcChannelOpened_;
};

#endif
