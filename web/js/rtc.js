function DataRTC() {
    // public var
    this.isCaller = true;
    this.channelName = null;

    // private var
    this._peerConnection = null;
    this._dataChannel = null;
    this._IceServers = { "iceServers": [ createIceServer("stun:stun.l.google.com:19302"),
                                         createIceServer("stun:stun1.l.google.com:19302"),
                                         createIceServer("stun:stun2.l.google.com:19302"),
                                         createIceServer("stun:stun3.l.google.com:19302") ]}; 
                                         //createIceServer("turn:yacamera.com:5000?transport=udp", "yacamera", "******")] };

    // callback
    this.onRTCMessageOut = null;
    this.onRTCData = null;
    this.onRTCState = null;
    this.onRTCError = null;

    // public interface
    this.initWithCall = function(channelName) {
        var that = this;
        this.isCaller = true;
        this.channelName = channelName;
        this._init();

        this._peerConnection.createOffer(this._onLocalDescription, function(err) {
            if (that.onRTCError) {
                that.onRTCError("createOffer", err);
            }
        });    // no MediaConstraints

    }.bind(this);

    this.initWithAnswer = function() {
        this.isCaller = false;
        this._init();
    }.bind(this);

    this.release = function() {
        this._peerConnection.close();
        delete this._peerConnection;
    }.bind(this);

    this.sendMessage = function(msg) {
        this._dataChannel.send(msg);
    }.bind(this);

    this.processRTCMessage = function(msg) {
        var that = this;
        if ( msg.length === 3 && msg[0] === "rtc" && msg[1] === "desc" ) {
            var desc = Base64.decode( msg[2] );
            var remoteDesc = new RTCSessionDescription(JSON.parse(desc));

            remoteDesc.sdp = remoteDesc.sdp.replace("AS:30", "AS:2000");

            this._peerConnection.setRemoteDescription( remoteDesc , function() {
                if ( that.isCaller === false) {
                    //{ 'mandatory': { 'OfferToReceiveAudio': true, 'OfferToReceiveVideo': true } });
                    that._peerConnection.createAnswer( that._onLocalDescription , function(err) {
                        if ( that.onRTCError !== null) {
                            that.onRTCError("setRemoteDescription", err);
                        }
                    }, null);
                }
            }, function(err) {
                if ( that.onRTCError !== null) {
                    that.onRTCError("setRemoteDescription", err);
                }
            });

        } else if ( msg.length === 3 && msg[0] === "rtc" && msg[1] === "cand" ) {
            var cand = Base64.decode(msg[2]);
            var candObj =  new RTCIceCandidate(JSON.parse(cand));
            //console.log(candObj);
            this._peerConnection.addIceCandidate(candObj);
        }

    }.bind(this);

    // callbacks from PeerConnection and DataChannel objects
    this._onLocalDescription = function(desc) {
        var that = this;

        desc.sdp = desc.sdp.replace("AS:30", "AS:2000");

        this._peerConnection.setLocalDescription(desc, function() {
            var descJsonObj = JSON.stringify(desc);
            var descBE = Base64.encode (descJsonObj);
            that.onRTCMessageOut("rtc:desc:" + descBE );
        }, function(err) {
            console.log(err);
            if ( that._onRTCError !== null) {
                that._onRTCError("setLocalDescription", err);
            }
        });
    }.bind(this);

    this._onLocalCandidate = function(evt) {
        if ( evt.candidate ) {
            var candJsonObj = JSON.stringify(evt.candidate);
            //console.log(">>>");
            //console.log(evt.candidate);
            var candBE = Base64.encode(candJsonObj);
            this.onRTCMessageOut("rtc:cand:" + candBE);
        }
    }.bind(this);

    this._onAddedDataChannel = function(evt) {
        this._dataChannel = evt.channel;
        this._dataChannel.onopen = this._onDataChannelOpen;
        this._dataChannel.onclose = this._onDataChannelClose;
        this._dataChannel.onerror = this._onDataChannelError;
        this._dataChannel.onmessage = this._onDataChannelMessage;
    }.bind(this);

    this._onDataChannelOpen = function() {
        if ( this.onRTCState !== null) {
            this.onRTCState("open");
        }
    }.bind(this);

    this._onDataChannelClose = function() {
        if ( this.onRTCState !== null) {
            this.onRTCState("close");
        }
    }.bind(this);

    this._onDataChannelError = function(evt) {
        if ( this.onRTCError !== null) {
            this.onRTCError("datachannel", evt);
        }
    }.bind(this);

    this._onDataChannelMessage = function(evt) {
        if ( this.onRTCData !== null) {
            this.onRTCData(evt.data);
        }
    }.bind(this);

    this._init = function() {
        this._peerConnection = new RTCPeerConnection(this._IceServers,  {optional: [{RtpDataChannels: true}]});
        this._peerConnection.onicecandidate = this._onLocalCandidate;

        if ( this.isCaller === true) {
            this._dataChannel = this._peerConnection.createDataChannel(this.channelName, {reliable: false});
            this._dataChannel.onopen = this._onDataChannelOpen;
            this._dataChannel.onclose = this._onDataChannelClose;
            this._dataChannel.onerror = this._onDataChannelError;
            this._dataChannel.onmessage = this._onDataChannelMessage;
        } else {
            this._peerConnection.ondatachannel = this._onAddedDataChannel;
        }
    }.bind(this);
};
