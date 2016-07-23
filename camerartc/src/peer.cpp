#include <iostream>
#include "webrtc/base/socketstream.h"
#include "webrtc/base/asyncsocket.h"

#include "helper.h"
#include "peer.h"

void Peer::PushMessage(const std::string& msg) {
    for(int i = 0; i < msg.size(); i++) {
        xmlBuffer.push_back( msg[i] );
        if ( msg[i] == '>') {
            processXML();
            xmlBuffer.clear();
        }
    }            
}

void Peer::processXML() {
    std::vector<std::string > words;

    words.clear();
    std::string currentWord = "";
    for(int i = 1; i < (int)xmlBuffer.size(); i++) {
        if ( xmlBuffer[i] == ':' || xmlBuffer[i] == '>') {
            words.push_back(currentWord);
            currentWord = "";
        } else {
            currentWord = currentWord.append(1, xmlBuffer[i]);
        }
    }

    if ( words.size() == 2 && words[0] == "login" && words[1] == "ok" ) {
        SignalOnline(true);
    } else if ( words.size() == 2 && words[0] == "online") {
        SignalPeerOnline( words[1] );
    } else if ( words.size() == 2 && words[0] == "offline") {
        SignalPeerOffline( words[1] );
    } else if ( words.size() >= 3 && words[0] == "message") {
        std::vector<std::string > msgBody;
        for(int j = 2; j < (int)words.size(); j++)
            msgBody.push_back(words[j]);

        SignalPeerMessage(words[1], msgBody);
    }
}

SocketPeer::SocketPeer(const std::string &server, const unsigned short port, const std::string& id, rtc::Thread *worker_thread) {
    server_address_ = server;
    server_port_ = port;
    id_ = id;
    isOnline_ = false;
    sock_ = NULL;

    worker_thread_ = worker_thread;

    SignalOnline.connect(this, &SocketPeer::onLoginEvent);
}

SocketPeer::~SocketPeer() {
    if ( sock_ ) {
        sock_->Close();
        delete sock_;
    }
}

void SocketPeer::OnMessage(rtc::Message *msg) {
    switch (msg->message_id) {
        case MSG_START:
            onStart_w();
    }
}

int SocketPeer::Login() {
    isOnline_ = false;
    worker_thread_->Post(this, MSG_START);
    return 0;
}

int SocketPeer::SendMessage(const std::string &to, const MsgBody& msg) {
    if ( !isOnline_ ) {
        return -1;
    }

    std::string msgPayload = "<message:" + to;
    for(int i = 0; i < (int)msg.size(); i++) {
        msgPayload = msgPayload + ":" + msg[i];
    }
    msgPayload = msgPayload + ">";

    sock_->Send( msgPayload.c_str(), msgPayload.size() );

    return 0;
}

void SocketPeer::onStart_w() {

    // Creating socket
    rtc::Thread* pth = rtc::Thread::Current();
    sock_ = pth->socketserver()->CreateAsyncSocket(SOCK_STREAM);
    sock_->SignalConnectEvent.connect(this, &SocketPeer::onConnectEvent);
    sock_->SignalReadEvent.connect(this, &SocketPeer::onReadEvent);
    sock_->SignalCloseEvent.connect(this, &SocketPeer::onCloseEvent);

    xmlBuffer.clear();

    // Connect to server
    rtc::SocketAddress addr(server_address_, server_port_);
    if (sock_->Connect(addr) < 0 &&  !sock_->IsBlocking() ) {
         sock_->Close();
         delete sock_;
         sock_ = NULL;

         SignalOnline(false);
    }
}

void SocketPeer::onLoginEvent(bool) {
    isOnline_ = true;
}

void SocketPeer::onConnectEvent(rtc::AsyncSocket* socket) {
    if ( sock_->GetState() == rtc::Socket::CS_CONNECTED) {
        //std::string loginMessage = "<signin:" + id_ + ":" + role_ + ">";
        //sock_->Send(loginMessage.c_str(), loginMessage.size() );
    }
}

void SocketPeer::onCloseEvent(rtc::AsyncSocket* socket, int err) {

    sock_->SignalConnectEvent.disconnect(this);
    sock_->SignalReadEvent.disconnect(this);
    sock_->SignalCloseEvent.disconnect(this);

    sock_->Close();
    //delete sock_;         //FIXME , we can't delete the socket in the self's callback
    sock_ = NULL;

    if ( isOnline_ == true) {
        isOnline_ = false;
        SignalOffline();
    } else {
        SignalOnline(false);
    }
}

void SocketPeer::onReadEvent(rtc::AsyncSocket* socket) {
    unsigned char temp[2048];

    int ret = sock_->Recv(temp, sizeof(temp) - 1);
    if ( ret > 0) {
        temp[ret] = 0;

        std::string msg ( (const char *)temp );
        PushMessage( msg );
    }
}

