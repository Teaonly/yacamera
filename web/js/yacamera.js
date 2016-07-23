function YaCamera(cameraName, canvas) {
    // var
    this._peer = null;
    this._player = null;
    this._streamers = null;
    this._camera = null;
    this._medias = null;
    this._monitor = null;

    // callback
    this.onConnect = null;
    this.onDisconnect = null;
    this.onStream = null;

    // public functions
    this.release = function() {

    }.bind(this);

    // private functions
    this._onLogin = function(isOK) {
        if ( this.onConnect !== null) {
            this.onConnect(isOK);
        }
    }.bind(this);
    this._onLogout = function() {
        if (this.onDisconnect !== null) {
            this.onDisconnect();
        }
    }.bind(this);

    this._onRTCState = function(remote, st) {
        if ( st === true) {
            this._updateWithPresence(remote);
        }
    }.bind(this);

    this._onRemoteLogin = function(remote) {
        var streamer = new Streamer(remote, this._peer);
        streamer.onMediaData = this._onMediaData;
        streamer.onRTCStateUpdate = function(st) {
            this._onRTCState(streamer.name, st);
        }.bind(this);
        this._streamers[remote] = streamer;

    }.bind(this);

    this._onRemoteLogout = function(remote) {
        if ( this._streamers[remote] !== undefined) {
            this._streamers[remote].release();
            delete this._streamers[remote];
        }
    }.bind(this);

    this._onRemoteMessage = function(remote, message) {
        if ( this._streamers[remote] !== undefined ) {
            this._streamers[remote].processMessage(message);
        }
    }.bind(this);

    this._doPlay = function() {
        for(i = 0;  i < this._medias.length; i++) {
            if ( this._medias[i].isReady === true) {
                var media = this._medias[i];
                if ( (this._player.mediaSequence === -1) ||
                     (this._compareSequence(media.mediaSequence, this._player.mediaSequence) === 1) ) {
                    this._player.playMedia(media);
                    if ( this.onStream !== null) {
                        this.onStream(true);
                    }
                    break;
                }
            }
        }

    }.bind(this);

    this._updateWithPresence = function(targetStreamer) {
        var presence = "";
        if ( this._medias.length == 0) {
            presence = "-1?0";
        } else {

            if ( this._medias[0].isReady) {
                presence = presence + this._medias[0].mediaSequence + "?1";
            } else {
                presence = presence + this._medias[0].mediaSequence + "?0";
            }

            for(i = 1; i < this._medias.length; i++) {
                if ( this._medias[i].isReady) {
                    presence = presence + "," + this._medias[i].mediaSequence + "?1";
                } else {
                    presence = presence + "," + this._medias[i].mediaSequence + "?0";
                }
            }
        }

        presence = "@" + presence;

        if ( targetStreamer === undefined ) {
            for ( s in this._streamers) {
                var streamer = this._streamers[s];
                if ( streamer.isCamera === false) {
                    streamer.sendPacket(presence);
                }
            }
        } else {
            var streamer = this._streamers[targetStreamer];
            if ( streamer.isCamera === false) {
                streamer.sendPacket(presence);
            }
        }

    }.bind(this);

    this._findMedia = function (seq) {
        for(i = 0; i < this._medias.length; i++) {
            if ( this._medias[i].mediaSequence === seq) {
                return this._medias[i];
            }
        }
        return null;
    }.bind(this);

    this._compareSequence = function (seq1, seq2) {
        if ( seq1 < 128 && seq2 > 32768) {
            return 1;
        }
        if ( seq1 > 32768 && seq2 < 128) {
            return -1;
        }
        if ( seq1 === seq2) {
            return 0;
        }
        if ( seq1 > seq2 ) {
          return 1;
        }
        if ( seq1 < seq2) {
          return -1;
        }
    }.bind(this);

    this._insertMedia = function(media) {
        var pos = -1;
        for(i = 0; i < this._medias.length; i++) {
            if ( this._compareSequence(this._medias[i].mediaSequence, media.mediaSequence) === -1){
                pos = i;
            }
        }
        pos ++;
        if ( pos >= this._medias.length) {
            this._medias.push(media);
        } else {
            this._medias.slice(pos, 0, media);
        }

    }.bind(this);

    this._popMedia = function() {
        var media = this._medias[0];
        this._medias.shift();

        media.release();
        delete yaMedia;
    }.bind(this);

    this._onMediaData = function(remote, data) {
        if ( yaParser.newPacket(data) === false) {
            return;
        }

        targetMedia = this._findMedia(yaParser.mediaSequence);
        if ( targetMedia === null ) {
            targetMedia = new YaMedia(yaParser.mediaType, yaParser.mediaSequence, yaParser.K, yaParser.mediaLength, yaParser.pictureLength);
            targetMedia.pushSymbol(yaParser.esi, yaParser.packet);

            if ( this._medias.length >= 5) {
                this._popMedia();
            }

            this._insertMedia(targetMedia);

            this._updateWithPresence();
        } else {
            if ( targetMedia.isReady) {
                return;
            }

            targetMedia.pushSymbol(yaParser.esi, yaParser.packet);
            if ( targetMedia.symbolNumber > targetMedia.K ) {
                targetMedia.doDecode();
                if ( targetMedia.isReady === true) {
                    this._updateWithPresence();
                    this._doPlay();
                }
            }
        }
    }.bind(this);

    this._streamingTimer = function() {
        setTimeout(this._streamingTimer, 15);

        if ( this._medias.length === 0) {
            return;
        }

        for ( s in this._streamers) {
            streamer = this._streamers[s];
            if ( streamer.isCamera === true) {
                continue;
            }

            if ( streamer.state <= 0 ) {
                continue;
            }

            var targetSeq = -2;
            for(i = streamer.mediaInfos.length - 1;  i >= 0; i--) {
                minfo = streamer.mediaInfos[i];
                if ( minfo.state === 0) {
                    targetSeq = minfo.seq;
                } else {
                    break;
                }
            }

            if ( targetSeq === -2) {
                targetSeq = streamer.mediaInfos[ streamer.mediaInfos.length - 1].seq;
                targetSeq ++;
                if ( targetSeq == 65536) {
                  targetSeq = 0;
                }
            } else if ( targetSeq === -1) {
                targetSeq = this._medias[0].mediaSequence;

                // media 0 may be dropped before sent to others
                targetSeq ++;
                if ( targetSeq == 65536) {
                  targetSeq = 0;
                }
            }

            var selectMedia = -1;
            for (i = 0; i < this._medias.length; i++) {
                if ( this._medias[i].isReady === false) {
                    continue;
                }
                var seq = this._medias[i].mediaSequence
                if ( this._compareSequence(seq, targetSeq) >= 0) {
                    selectMedia = i;
                    break;
                }
            }

            if ( selectMedia === -1) {
                continue;
            }

            var packet = this._medias[i].buildPacket( streamer.eigenValue );
            streamer.sendPacket(packet);

            streamer.eigenValue ++;
            if ( streamer.eigenValue >= 1000) {
                streamer.eigenValue = 0;
            }
        }
    }.bind(this);

    this._updateTimer = function() {
        this._updateWithPresence();
        setTimeout(this._updateTimer, 500);
    }.bind(this);

    // constructor
    this._constructor = function() {

        this._peer = new Peer(cameraName, "V" + Math.floor(Math.random() * 10000), "ws://yacamera.com/ws/");
        //this._peer = new Peer(cameraName, "V" + Math.floor(Math.random() * 10000), "ws://10.3.4.106:1982/ws/");
        this._peer.onLogin = this._onLogin;
        this._peer.onLogout = this._onLogout;
        this._peer.onRemoteLogin = this._onRemoteLogin;
        this._peer.onRemoteLogout = this._onRemoteLogout;
        this._peer.onMessage = this._onRemoteMessage;

        this._streamers = {};
        this._medias = new Array();

        this._player = new Player(canvas, 8000);
        this._player.onStreamStarvation = function() {
            if ( this.onStream !== undefined) {
                this.onStream(false);
            }
        }.bind(this);

        setTimeout(this._streamingTimer, 15);
        setTimeout(this._updateTimer, 500);

        this._peer.login();
    }.bind(this);

    this._constructor();
};
