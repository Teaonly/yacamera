#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#include "helper.h"
#include "webrtc/base/base64.h"
#include "attendee.h"
#include "peer.h"
#include "rtcstream.h"

Attendee::Attendee(const std::string& attendeeName,
                   Peer* peer):
        peer_(peer), attendeeName_(attendeeName) {

    factory_ = webrtc::CreatePeerConnectionFactory();

    stream_ = new RtcStream(attendeeName_, factory_);
    stream_->SignalSessionDescription.connect(this, &Attendee::onLocalDescription);
    stream_->SignalIceCandidate.connect(this, &Attendee::onLocalCandidate);

    stream_->SignalChannelOpened.connect(this, &Attendee::onChannelOpened);
    stream_->SignalChannelClosed.connect(this, &Attendee::onChannelClosed);
    stream_->SignalChannelData.connect(this, &Attendee::onChannelData);
}

Attendee::~Attendee() {
    if ( stream_ != NULL) {
        delete stream_;
        stream_ = NULL;
    }
}

void Attendee::OnMessage(rtc::Message *msg) {

}

void Attendee::onLocalDescription(RtcStream* stream, const std::string& desc) {
    sendMessage("rtc", "desc" , desc);
}

void Attendee::onLocalCandidate(RtcStream* stream, const std::string& cand) {
    sendMessage("rtc", "cand", cand);
}

void Attendee::onChannelOpened(RtcStream* stream, const std::string& cname) {
    LOGD("Datachannel is created!");  
    if ( cname == "yacamera") {
        if ( rtcChannelOpened_ == false) {
            rtcChannelOpened_ = true;
        }
    }
}

void Attendee::onChannelClosed(RtcStream* stream) {
    rtcChannelOpened_ = false;
}

void Attendee::onChannelData(RtcStream* stream, const webrtc::DataBuffer& buffer) {

}

void Attendee::sendMessage(const std::string& v0, const std::string& v1) {
    MsgBody msg;
    msg.push_back(v0);
    msg.push_back(v1);
    peer_->SendMessage(attendeeName_, msg);
}

void Attendee::sendMessage(const std::string& v0, const std::string& v1, const std::string& v2) {
    MsgBody msg;
    msg.push_back(v0);
    msg.push_back(v1);
    msg.push_back(v2);
    peer_->SendMessage(attendeeName_, msg);
}

void Attendee::processMessage(const std::vector<std::string>& words) {
    if ( words.size() == 3 && words[0] == "rtc" && words[1] == "desc" ) {
        // call from remote
        stream_->SetRemoteDescription( words[2] );
        stream_->CreateAnswerDescription();
    } else if ( words.size() == 3 && words[0] == "rtc" && words[1] == "cand" ) {
        stream_->SetRemoteCandidate( words[2] );
    }
}

int Attendee::sendMediaPackage(unsigned char *payload, unsigned int length) {
    // Change YA binary to BASE64 data
    std::string yaMessage;
    rtc::Base64::EncodeFromArray(payload, length, &yaMessage);
    webrtc::DataBuffer yaData(yaMessage);
    bool ret = stream_->SendChannelData(yaData);
    return 0;
}

