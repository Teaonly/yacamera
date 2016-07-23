#include <iostream>
#include <sstream>

#include "talk/app/webrtc/peerconnectioninterface.h"
#include "webrtc/base/base64.h"
#include "webrtc/base/json.h"

#include "rtcstream.h"
#include "simpleconstraints.h"
#include "talk/app/webrtc/test/fakedtlsidentityservice.h"

static bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

// 
// Internal helper observer implementation. 
//
class RtcStreamSetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    static RtcStreamSetSessionDescriptionObserver* Create() {
        return
            new rtc::RefCountedObject<RtcStreamSetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {
        LOG(INFO) << "SetSessionDescription is Success";
    }
    virtual void OnFailure(const std::string& error) {
        LOG(WARNING) << "SetSessionDescription is failture";
    }

protected:
    RtcStreamSetSessionDescriptionObserver() {}
    ~RtcStreamSetSessionDescriptionObserver() {}

};

class RtcStreamCreateSessionDescriptionObserver
    : public webrtc::CreateSessionDescriptionObserver
    , public sigslot::has_slots<> {
public:
    static RtcStreamCreateSessionDescriptionObserver* Create() {
        return
            new rtc::RefCountedObject<RtcStreamCreateSessionDescriptionObserver>();
    }
    
    virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) {
        SignalCreatedSession(desc);
    } 
    
    virtual void OnFailure(const std::string& error) {
        SignalCreatedSession(NULL);
    }
    
    sigslot::signal1<webrtc::SessionDescriptionInterface*> SignalCreatedSession;

protected:
    RtcStreamCreateSessionDescriptionObserver() {}
    ~RtcStreamCreateSessionDescriptionObserver() {}
};

RtcStream::RtcStream(const std::string& id, 
            webrtc::PeerConnectionFactoryInterface* factory) : 
            id_(id), factory_(factory) {

    webrtc::PeerConnectionInterface::IceServers servers;
    
    webrtc::PeerConnectionInterface::IceServer stunServer;
    stunServer.uri = "stun:stun.l.google.com:19302";
    //stunServer.uri = "stun:stunserver.org:3478";
    servers.push_back(stunServer);
    stunServer.uri = "stun:stun.l.google.com:19302";
    servers.push_back(stunServer);
    stunServer.uri = "stun:stun1.l.google.com:19302";
    servers.push_back(stunServer);
    stunServer.uri = "stun:stun2.l.google.com:19302";
    servers.push_back(stunServer);

    webrtc::PeerConnectionInterface::IceServer turnServer;
    turnServer.uri = "turn:yacamera.com:5000?transport=udp";
    turnServer.username = "yacamera";
    turnServer.password = "111111";  
    
    servers.push_back(turnServer);

    FakeIdentityService* myDTLS = new FakeIdentityService();
    webrtc::SimpleConstraints myConstraints;
    myConstraints.SetAllowRtpDataChannels();    
    connection_ = factory_->CreatePeerConnection(servers, 
                                                 &myConstraints,
                                                 NULL,
                                                 myDTLS,
                                                 this); 
}

RtcStream::~RtcStream() {
    connection_->Close();
}

// PeerConnectionObserver callbackes
void RtcStream::OnAddStream(webrtc::MediaStreamInterface* stream) {
    /* 
    stream->AddRef();
    webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
    // Only render the first track.
    if (!tracks.empty()) {
        webrtc::VideoTrackInterface* track = tracks[0];
        if ( videoRenderer_ != NULL) {
            track->AddRenderer(videoRenderer_);
        }
    }
    stream->Release();
    */
}

void RtcStream::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
    LOG(INFO) << __FILE__ << "@" << __LINE__ << "OnRemoveStream";
}

void RtcStream::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    std::string cand;
    if (!candidate->ToString(&cand)) {
        LOG(LS_ERROR) << "Failed to serialize candidate";
        return;
    }

    Json::StyledWriter writer;
    Json::Value jmessage;
    jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
    jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
    jmessage[kCandidateSdpName] = cand;
    std::string encodedCand = rtc::Base64::Encode( writer.write(jmessage)); 
    SignalIceCandidate(this, encodedCand);
}


void RtcStream::OnDataChannel(webrtc::DataChannelInterface* data_channel) {
    data_channel_ = data_channel;
    data_channel_->RegisterObserver(this);
}

void RtcStream::OnStateChange() {
    if ( data_channel_.get() != NULL) {
        webrtc::DataChannelInterface::DataState st = data_channel_->state();
        if ( st == webrtc::DataChannelInterface::kOpen ) {
            SignalChannelOpened(this, data_channel_->label());              
        } else if ( st == webrtc::DataChannelInterface::kClosed) {
            SignalChannelClosed(this);
        }
    }
}

void RtcStream::OnMessage(const webrtc::DataBuffer& buffer) {
    SignalChannelData(this, buffer);
}

void RtcStream::OnLocalDescription(webrtc::SessionDescriptionInterface* desc) {
    if ( desc != NULL) {

        Json::StyledWriter writer;
        Json::Value jmessage;
        jmessage[kSessionDescriptionTypeName] = desc->type();
        std::string sdp;
        desc->ToString(&sdp);
        replace(sdp, "AS:30", "AS:2000");       //Hacking: change bandwidth to 2M
        jmessage[kSessionDescriptionSdpName] = sdp;

        webrtc::SessionDescriptionInterface* sd(
            webrtc::CreateSessionDescription(desc->type(), sdp));
        connection_->SetLocalDescription(    
                RtcStreamSetSessionDescriptionObserver::Create(), sd);
    
        std::string encodedDesc = rtc::Base64::Encode( writer.write(jmessage) );
        SignalSessionDescription(this, encodedDesc); 
    } else {
        LOG(WARNING) << "Create local sessiondescription error!"; 
    }    
} 

bool RtcStream::SendChannelData(const webrtc::DataBuffer& buffer) {
    if ( data_channel_.get() != NULL) {
        return data_channel_->Send(buffer);
    }
    return false;
}

bool RtcStream::IsChannelOpened() {
    if ( data_channel_.get() != NULL) {
        webrtc::DataChannelInterface::DataState st = data_channel_->state();
        if ( st == webrtc::DataChannelInterface::kOpen ) {
            return true;
        }
    }
    
    return false;
}

void RtcStream::SetRemoteCandidate(const std::string& msg) {
    // convert to local json object
    Json::Reader reader;
    Json::Value jmessage;
    std::string decodedMsg = rtc::Base64::Decode(msg, rtc::Base64::DO_STRICT);     
    if (!reader.parse(decodedMsg, jmessage)) {
        LOG(WARNING) << " Parse the JSON failed";
        return;
    }

    // parse the received json object
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!GetStringFromJsonObject(jmessage, kCandidateSdpMidName, &sdp_mid) ||
            !GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                    &sdp_mlineindex) ||
            !GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
        LOG(WARNING) << "Can't parse received message.";
        return;
    }   
    
    rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate( 
                webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp));
    if (!candidate.get()) {
        LOG(WARNING) << "Can't parse received candidate message.";
        return;
    }   
    
    if (!connection_->AddIceCandidate(candidate.get())) {
        LOG(WARNING) << "Failed to apply the received candidate";
        return;
    }   
    
    return;
}

void RtcStream::SetRemoteDescription(const std::string& msg) {
    // convert to local json object
    Json::Reader reader;
    Json::Value jmessage;
    std::string decodedMsg = rtc::Base64::Decode(msg, rtc::Base64::DO_STRICT);     
    replace(decodedMsg, "AS:30", "AS:2000");        //Hacking, Change default bandwidth to 2M
    if (!reader.parse(decodedMsg, jmessage)) {
        LOG(WARNING) << " Parse the JSON failed";
        return;
    }
    // convert to local sesion description object
    std::string type;
    std::string sdp;
    GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
    GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName, &sdp);
    if ( sdp.empty() || type.empty() ) {
        LOG(WARNING) << " Convert to SDP failed";
        return;
    }

    webrtc::SessionDescriptionInterface* session_description(
            webrtc::CreateSessionDescription(type, sdp));
    if (!session_description) {
        LOG(WARNING) << "Can't parse received session description message."; 
        return;
    }

    connection_->SetRemoteDescription(
            RtcStreamSetSessionDescriptionObserver::Create(), session_description);
}

void RtcStream::SetupDataChannel(const std::string& cname) {
    webrtc::DataChannelInit dcSetting;
    dcSetting.reliable = false;
    dcSetting.ordered = false;
    data_channel_ = connection_->CreateDataChannel(cname, &dcSetting);
    data_channel_->RegisterObserver(this);
}

void RtcStream::CreateOfferDescription() {
    RtcStreamCreateSessionDescriptionObserver *ob = RtcStreamCreateSessionDescriptionObserver::Create();
    ob->SignalCreatedSession.connect(this, &RtcStream::OnLocalDescription);
    connection_->CreateOffer(ob, NULL);
}

void RtcStream::CreateAnswerDescription() {
    RtcStreamCreateSessionDescriptionObserver *ob = RtcStreamCreateSessionDescriptionObserver::Create();
    ob->SignalCreatedSession.connect(this, &RtcStream::OnLocalDescription);
    connection_->CreateAnswer(ob, NULL);
}

