#ifndef _PEER_H_
#define _PEER_H_

#include <string>
#include <vector>

#include "webrtc/base/thread.h"
#include "webrtc/base/messagequeue.h"

namespace rtc{
    class AsyncSocket;
};

typedef std::vector<std::string> MsgBody;

class Peer {
public:
    Peer() {}; 
    virtual ~Peer(){};

    virtual int Login() = 0;
    virtual int SendMessage(const std::string &, const MsgBody& ) = 0;

    void PushMessage(const std::string& msg);

    sigslot::signal1<bool> SignalOnline;
    sigslot::signal0<> SignalOffline;
    sigslot::signal1<const std::string &> SignalPeerOnline;
    sigslot::signal1<const std::string &> SignalPeerOffline;
    sigslot::signal2<const std::string&, const std::vector<std::string>& > SignalPeerMessage;

private:
    void processXML();
    
protected:

    std::vector<char> xmlBuffer;
};

class SocketPeer : public sigslot::has_slots<>, public rtc::MessageHandler, public Peer {
public:
    SocketPeer(const std::string &server, const unsigned short port, const std::string& id, rtc::Thread *worker_thread);
    ~SocketPeer();

    // helper and fast access
    std::string server_address() const {
        return server_address_;
    }
    unsigned short server_port() const {
        return server_port_;
    }
    rtc::Thread* WorkerThread() {
        return worker_thread_;
    }

    virtual int Login();
    virtual int SendMessage(const std::string &, const std::vector<std::string>& );

protected:
    virtual void OnMessage(rtc::Message *msg);

    void onStart_w();
    void onLoginEvent(bool);
    void onConnectEvent(rtc::AsyncSocket* socket);
    void onCloseEvent(rtc::AsyncSocket* socket, int err);
    void onReadEvent(rtc::AsyncSocket* socket);
    void processXML();

private:
    enum {
        MSG_START,
    };
    std::string id_;
    rtc::Thread*  worker_thread_;
    rtc::AsyncSocket* sock_;

    bool isOnline_;
    std::string server_address_;
    unsigned short server_port_;

    std::vector<char> xmlBuffer;
};

#endif
