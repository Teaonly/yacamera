#ifndef _RTCSTREAM_H_
#define _RTCSTREAM_H_

#include <string>
#include "talk/app/webrtc/peerconnectionfactory.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "talk/session/media/channelmanager.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread.h"

class RtcStream : public webrtc::PeerConnectionObserver,
                  public webrtc::DataChannelObserver,
                  public sigslot::has_slots<> {
public:
    RtcStream(const std::string& id, webrtc::PeerConnectionFactoryInterface* factory);
    virtual ~RtcStream();

    inline std::string id() { return id_; }

    // Signaling interfaces
    virtual void CreateOfferDescription();
    virtual void CreateAnswerDescription();
    virtual void SetRemoteCandidate(const std::string& msg);
    virtual void SetRemoteDescription(const std::string& msg);
    virtual void SetupDataChannel(const std::string& cname);
    sigslot::signal2<RtcStream*, const std::string&> SignalSessionDescription;
    sigslot::signal2<RtcStream*, const std::string&> SignalIceCandidate;

    // Data interfaces
    bool SendChannelData(const webrtc::DataBuffer&);
    bool IsChannelOpened();
    sigslot::signal2<RtcStream*, const std::string&> SignalChannelOpened;
    sigslot::signal1<RtcStream*> SignalChannelClosed;
    sigslot::signal2<RtcStream*, const webrtc::DataBuffer&> SignalChannelData;

protected:
    //
    // PeerConnectionObserver implementation.
    //
    virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
    virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
    virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
    virtual void OnDataChannel(webrtc::DataChannelInterface* data_channel);

    virtual void OnRenegotiationNeeded() {}
    virtual void OnStateChange(
            webrtc::PeerConnectionObserver::StateType state_changed) {}
    virtual void OnIceConnectionChange(
            webrtc::PeerConnectionInterface::IceConnectionState new_state) {}
    virtual void OnIceGatheringChange(
            webrtc::PeerConnectionInterface::IceGatheringState new_state) {}
    virtual void OnError() {}

    //
    // DataChannelObserver implementation
    //
    virtual void OnStateChange();
    virtual void OnMessage(const webrtc::DataBuffer& buffer);

    //
    // Callback from CreateSessionDescriptionObserver
    //
    virtual void OnLocalDescription(webrtc::SessionDescriptionInterface* desc);

protected:
    std::string id_;

    webrtc::PeerConnectionFactoryInterface* factory_;

    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
};

#endif
