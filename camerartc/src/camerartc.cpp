#include <assert.h>
#include <sstream>
#include <iostream>

#include "webrtc/common_audio/resampler/include/resampler.h"

#include "camerartc.h"
#include "peer.h"
#include "rtcstream.h"
#include "attendee.h"
#include "mediabuffer.h"
#include "raptor/raptor.h"
#include "helper.h"

enum {
    MSG_STREAMING_TIMER = 1,
    MSG_PEER_ONLINE,
    MSG_PEER_OFFLINE,
    MSG_PEER_MESSAGE,
};

const float FEC_REDUNDANCY = 1.5;
const int RAW_PACKAGE_NUMBER = 128;
const int RAW_PAYLOAD_SIZE = 860;
const int REAL_PAYLOAD_SIZE = 840;
const int STREAMING_TIMER_VALUE = 15;

struct PeerMessageData : public rtc::MessageData {
    std::string peer_;
    std::vector<std::string> words_;
};

void CameraRtc::init(const std::string& name) {
    my_name_ = name;
    mediaPool_ = new MediaBuffer(RAW_PACKAGE_NUMBER, RAW_PAYLOAD_SIZE);
    mediaSequence_ = 0;
    isStreaming_ = false;
    
    g726_init_state(&g726State_);
    resampler_ = new webrtc::Resampler(16000, 8000, webrtc::kResamplerSynchronous); 
    adpcmLength_ = 0;

    signal_thread_ = new rtc::Thread();
    signal_thread_->Start();
}


CameraRtc::CameraRtc(const std::string& myName, const std::string& server, const int port) {
    init(myName);
    
    peer_ =  new SocketPeer(server, port, my_name_, signal_thread_);
    peer_->SignalOnline.connect(this, &CameraRtc::onOnline);
    peer_->SignalOffline.connect(this, &CameraRtc::onOffline);
    peer_->SignalPeerOnline.connect(this, &CameraRtc::onPeerOnline);
    peer_->SignalPeerOffline.connect(this, &CameraRtc::onPeerOffline);
    peer_->SignalPeerMessage.connect(this, &CameraRtc::onPeerMessage);
}

CameraRtc::CameraRtc(const std::string& myName, Peer *peer) {
    init(myName);
    peer_ = peer;

    peer_->SignalOnline.connect(this, &CameraRtc::onOnline);
    peer_->SignalOffline.connect(this, &CameraRtc::onOffline);
    peer_->SignalPeerOnline.connect(this, &CameraRtc::onPeerOnline);
    peer_->SignalPeerOffline.connect(this, &CameraRtc::onPeerOffline);
    peer_->SignalPeerMessage.connect(this, &CameraRtc::onPeerMessage);
}

CameraRtc::~CameraRtc() {
    delete peer_;
    
    if ( signal_thread_ ) {
        signal_thread_->Stop();
        delete signal_thread_;
    } 

    delete mediaPool_;
    delete resampler_;
}

void CameraRtc::Login() {
   peer_->Login();
}

void CameraRtc::OnMessage(rtc::Message *msg) {
    switch (msg->message_id) {
        case MSG_STREAMING_TIMER:
            streamingTimer_s();
            signal_thread_->PostDelayed(STREAMING_TIMER_VALUE, this, MSG_STREAMING_TIMER); 
            break;
        
        case MSG_PEER_ONLINE: {
                rtc::TypedMessageData<std::string> *data =
                    static_cast<rtc::TypedMessageData<std::string>*>(msg->pdata);
                peerOnline_s(data->data());
            }
            break;
 
        case MSG_PEER_OFFLINE: {
                rtc::TypedMessageData<std::string> *data =
                    static_cast<rtc::TypedMessageData<std::string>*>(msg->pdata);
                peerOffline_s(data->data());
            }
            break;
    
        case MSG_PEER_MESSAGE: {
                PeerMessageData* data = static_cast<PeerMessageData*>(msg->pdata);
                peerMessage_s(data->peer_, data->words_);
           }
           break;

    }
}

void CameraRtc::onOnline(bool isOk) {
    if ( isStreaming_ == false) {
        isStreaming_ = true;
        signal_thread_->PostDelayed(STREAMING_TIMER_VALUE, this, MSG_STREAMING_TIMER); 
    }
}

void CameraRtc::onOffline() {
    
}

void CameraRtc::onPeerOnline(const std::string& peer) {
    signal_thread_->Post(this, MSG_PEER_ONLINE, 
        new rtc::TypedMessageData<std::string>(peer) );
}

void CameraRtc::peerOnline_s(const std::string& peer) {
    if ( onlines_.find(peer) != onlines_.end()) {
        assert(false);
    }
    
    Attendee* newViewer = new Attendee(peer, peer_);
    onlines_[peer] = newViewer;
}

void CameraRtc::onPeerOffline(const std::string& peer) {
    signal_thread_->Post(this, MSG_PEER_OFFLINE, 
        new rtc::TypedMessageData<std::string>(peer) );
}

void CameraRtc::peerOffline_s(const std::string& peer) {
    if ( onlines_.find(peer) == onlines_.end() ) {
        assert(false);
        return;
    }

    delete onlines_[peer];
    onlines_.erase(peer);
}


void CameraRtc::onPeerMessage(const std::string& remote, const std::vector<std::string>& words) {
    PeerMessageData* data = new PeerMessageData();
    data->peer_ = remote;
    data->words_ = words;
    signal_thread_->Post(this, MSG_PEER_MESSAGE, data);
}

void CameraRtc::peerMessage_s(const std::string& remote, const std::vector<std::string>& words) {
    if ( onlines_.find(remote) == onlines_.end() ) {
        assert(false);
        return;
    }
    onlines_[remote]->processMessage(words);
}

void CameraRtc::UpdatePicture(unsigned char *jpegData, int length) {
    static unsigned char localBuffer[1024*64];
    if ( isStreaming_ == false) {
        rtc::CritScope lock(&adpcmMutex_);
        adpcmLength_ = 0;
        return;
    }
    
    // combinating jpeg and g.271 audio
    int totalLength = 0;
    if ( length >= sizeof(localBuffer) )
        return;
    memcpy(localBuffer, jpegData, length);
    { 
        rtc::CritScope lock(&adpcmMutex_);
        if ( sizeof(localBuffer) - length < adpcmLength_) {
            adpcmLength_ = 0;
            return;
        }
        memcpy(&localBuffer[length], adpcmBuffer_, adpcmLength_);
        totalLength = length + adpcmLength_; 
        adpcmLength_ = 0;
    }

    
    int k = (totalLength + REAL_PAYLOAD_SIZE - 1) / REAL_PAYLOAD_SIZE;
    int fecNumber = k * 1.0 * FEC_REDUNDANCY;
    if ( mediaPool_->FreeSize() < fecNumber ) {
        LOGD(">>>> buffer over flow, drop media data>>>");
        return;
    }
    
    struct RaptorParameter param = TeaonlyRaptor::createRaptorParameter(k);
    TeaonlyRaptor raptorEncoder(param);
    raptorEncoder.doEncode((unsigned char *)localBuffer, totalLength);

    unsigned char buffer[RAW_PAYLOAD_SIZE]; 
    RaptorSymbol symbol;
    symbol.esi = 0;
    for(int i = 0; i < fecNumber; i++) {
        *(unsigned short *)&buffer[0] = 0x1980;
        *(unsigned short *)&buffer[2] = mediaSequence_;
        *(unsigned short *)&buffer[4] = (unsigned short)k;
        *(unsigned short *)&buffer[6] = (unsigned short)symbol.esi;
        *(unsigned int *)&buffer[8] = totalLength;
        *(unsigned int *)&buffer[12] = length;

        raptorEncoder.getSymbol(&symbol);
        unsigned char* payload = &symbol.data[0];
        memcpy( &buffer[16], payload,  symbol.data.size() );
        mediaPool_->PushBuffer(&buffer[0], 16 + symbol.data.size()); 
        
        symbol.esi ++;
    } 
    
    mediaSequence_ ++;
}

void CameraRtc::UpdatePCM(unsigned char *pcmData, int length) {
    static unsigned char localBuffer[1024*16];
    int rLength;
    if ( resampler_->Push(( short*)pcmData, length/2, (short*)localBuffer, sizeof(localBuffer), rLength) < 0) {
        return;
    }
    
    int j = 0;
    unsigned char byteCode = 0x00;
    for(int i = 0; i < rLength; i++) {
        short pcm = ((short *)&localBuffer[0])[i];
        unsigned char code = g726_32_encoder(pcm, AUDIO_ENCODING_LINEAR, &g726State_);
        
        if (i & 0x01) {
            byteCode = (code << 4) | (byteCode & 0x0f);
            localBuffer[j] = byteCode;
            j++;
        } else {
            byteCode = code;
        }
    }
    
    { 
        rtc::CritScope lock(&adpcmMutex_);
        if ( (sizeof(adpcmBuffer_) - adpcmLength_) > j) {
            memcpy( &adpcmBuffer_[adpcmLength_], localBuffer, j);    
            adpcmLength_ += j;
        } 
    }

}

void CameraRtc::streamingTimer_s() {

    if ( isStreaming_ == false)
        return;

    if ( mediaPool_->PullBuffer() == false ) {
        return;
    }

    MediaPackage* pkg = mediaPool_->Released();

    for( std::map<std::string, Attendee*>::iterator i = onlines_.begin(); i != onlines_.end(); i++) {
       i->second->sendMediaPackage(pkg->data, pkg->length); 
    }

}
