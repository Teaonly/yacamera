#include <iostream>

#include "jnipeer.h"

JniPeer::JniPeer(const std::string& cname, int fd) : cameraName_(cname), jfd_(fd) { 
     
}

JniPeer::~JniPeer() {

}

int JniPeer::Login() {
    return 1;
}

int JniPeer::SendMessage(const std::string& name, const std::vector<std::string>& msg) {
    std::string msgPayload = "<message:" + name;
    for(int i = 0; i < (int)msg.size(); i++) {
        msgPayload = msgPayload + ":" + msg[i];
    }
    msgPayload = msgPayload + ">";

    send(jfd_, msgPayload.c_str(), msgPayload.size(), 0);  
    return 0;
}

int JniPeer::UpdatePresence(const std::string& presence) {
    std::string msgPayload = "<presence:" + presence + ">";
    send(jfd_, msgPayload.c_str(), msgPayload.size(), 0);

    return 0;   
}

void JniPeer::onDisconnect() {
    SignalOffline(); 
}


