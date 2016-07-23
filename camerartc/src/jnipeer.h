#ifndef _JNIPEER_H_
#define _JNIPEER_H_

#include "peer.h"

class JniPeer: public Peer {
public:
    JniPeer(const std::string& cname, int fd);
    ~JniPeer();
   
    virtual int Login();
    virtual int SendMessage(const std::string &, const std::vector<std::string>& );
    virtual int UpdatePresence(const std::string& presence);
    
    void onDisconnect();

private:
    std::string cameraName_; 
    int jfd_;
};

#endif
