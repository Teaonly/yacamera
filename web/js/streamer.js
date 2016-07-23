function Streamer(name, peer) {
    // vars
    this.name = null;
    this._peer = null;
    this._rtc = null;
    this.state = 0;    //-1: rtc is close; 0: rtc is not ready; 1: rtc is ready
    this.eigenValue = -1;

    this.mediaInfos = null;
    this.isCamera = false;

    // call back
    this.onRTCStateUpdate = null;
    this.onMediaData = null;

    // public functions
    this.update = function(presence) {
        var seqs = presence.split(",");

        this.mediaInfos = new Array();
        for(i = 0; i < seqs.length; i++) {
            m = seqs[i].split("?");
            var minfo = {};
            minfo.seq = parseInt(m[0]);
            minfo.state = parseInt(m[1]);
            this.mediaInfos.push(minfo);
        }
    }.bind(this);

    this.processMessage = function(message) {
        if ( message[0] === "rtc") {
            this._rtc.processRTCMessage(message);
        }
    }.bind(this);

    this.release = function() {
        this._rtc.release();
        delete this._rtc;
    }.bind(this);

    this.sendPacket = function(packet) {
        this._rtc.sendMessage(packet);
    }.bind(this);

    // private functions
    this._onRTCMessageOut = function(msg) {
        this._peer.sendMessage(this.name, msg);
    }.bind(this);

    this._onRTCState = function(state) {
        if ( state === "open" ) {
            if ( this.state !== 1) {
                this.state = 1;
                if ( this.onRTCStateUpdate !== null) {
                    this.onRTCStateUpdate(true);
                }
            }
        }
    }.bind(this);
    this._onRTCError = function(type, evt) {
        if ( this.state !== -1) {
            this.state = -1;
            if ((this.onRTCStateUpdate !== null) ) {
                this.onRTCStateUpdate(false);
            }
        }
        this.state = -1;
    }.bind(this);

    this._onRTCData = function(data) {
        if ( data[0] === '@') {
            var presence = data.substring(1);
            this.update(presence);
        } else if ( this.onMediaData !== null) {
            this.onMediaData(this.name, data);
        }
    }.bind(this);

    // constroctor
    this._constructor = function() {
        this.name = name;
        this.state = 0;
        this._peer = peer;

        this.eigenValue = Math.floor(Math.random() * 1000);

        this._rtc = new DataRTC();
        this._rtc.onRTCMessageOut = this._onRTCMessageOut;
        this._rtc.onRTCState = this._onRTCState;
        this._rtc.onRTCError = this._onRTCError;
        this._rtc.onRTCData = this._onRTCData;

        this.isCamera = false;
        if ( this._peer.cameraName === this.name ) {
            this.isCamera = true;
            this._rtc.initWithCall("yacamera");
        } else if ( this.name.localeCompare(this._peer.peerName) === 1) {
            this._rtc.initWithCall("yacamera");
        } else {
            this._rtc.initWithAnswer();
        }

    }.bind(this);

    this._constructor();
};
