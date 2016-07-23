function Player (canvas, sampleRate) {
    // vars
    this.mediaSequence = -1;
    this._canvas = null;
    this._canvasContext = null;
    this._rgba = null;
    
    this._avc = null;
    this._videoBufferList = null;

    this._sampleRate = -1;
    this._audioCtx = null;
    this._beginTime = -1;
    this._audioBufferList = null;

    // call backs
    this.onStreamStarvation = null;

    // public functions
    this._constructor = function() {
        this._nextMediaSequence = -1;

        this._canvas = canvas;
        //this._canvasContext = this._canvas.getContext("2d");

        window.AudioContext = window.AudioContext || window.webkitAudioContext;
        this._audioCtx = new AudioContext();
        this._sampleRate = sampleRate;
        this._audioBufferList = new Array();

        this._avc = new Avc();
        this._avc.onPictureDecoded = function(buffer, wid, hei) {
            var yuv = new Uint8Array(buffer.length);
            yuv.set( buffer, 0, buffer.length);    
            this._pushVideoBuffer({'yuv':yuv, 'wid':wid, 'hei':hei}, 1);
        }.bind(this);
 
        this._videoBufferList = new Array();

    }.bind(this);

    this.playMedia = function(media) {

        console.log(">>>>> Media: seq = " + media.mediaSequence + " length = " + media.pictureLength);

        this.mediaSequence = media.mediaSequence;

        var payload = media.getAudioPayload();
        var audioDecoder = new AdpcmDecoder();
        pcmData = audioDecoder.doDecode(payload);
        audioDecoder.release();
        delete audioDecoder;

        this._pushAudioBuffer(pcmData);

        payload = media.getPicturePayload();
        if ( media.mediaType === 0) {
            this._pushVideoBuffer(payload, 0);
        } else if ( media.mediaType === 1) {
            this._avc.decode(payload);
        }

    }.bind(this);

    // private functions
    this._pushAudioBuffer = function(pcmData, picture, mediaType) {
        var source = this._createAudioSource(pcmData);
        source.connect(this._audioCtx.destination);

        source.onended = function(evt) {
            this._audioBufferList.shift();
            if ( (this._audioBufferList.length === 0) && (this.onStreamStarvation !== null) ) {
                this.onStreamStarvation();
            }
            this._beginTime = this._audioCtx.currentTime;
        }.bind(this);

        var bufferedDuration = 0;
        var leftDuration = 0;
        if ( this._audioBufferList.length > 0) {
            for (i = 1; i < this._audioBufferList.length; i++) {
                bufferedDuration += this._audioBufferList[i].buffer.duration;
            }
            leftDuration = this._audioCtx.currentTime - this._beginTime;
        } else {
            this._beginTime = this._audioCtx.currentTime;
        }
        this._audioBufferList.push(source);

        source.start(leftDuration + bufferedDuration);
    }.bind(this);

    this._createAudioSource = function(data) {
        // create pcm buffer
        var pcmShort = new Int16Array(data.length/2);
        for(var i = 0, j = 0; i < data.length; i+=2)  {
            pcmShort[j] = (data[i] & 0xFF) | ((data[i+1] & 0xff) << 8);
            j++;
        }
        var pcmFloat = new Float32Array(pcmShort.length);
        for(var i = 0; i < pcmFloat.length; i++) {
            pcmFloat[i] = pcmShort[i] / 32768;
        }
        delete pcmShort;
        var audioBuffer = this._audioCtx.createBuffer(1, pcmFloat.length, this._sampleRate);
        audioBuffer.getChannelData(0).set(pcmFloat);

        // create play source node
        var source = this._audioCtx.createBufferSource();
        source.buffer = audioBuffer;
        return source;
    }.bind(this);

    this._showPicture = function(picture) {
        if (picture.t === 0) {
            var payload = picture.payload;
            var b64jpeg = btoa(String.fromCharCode.apply(null, payload));
            var img = new Image();
            img.src = "data:image/jpeg;base64," + b64jpeg;
            img.onload = function () {
                this._canvasContext.drawImage(img, 0, 0, this._canvas.width, this._canvas.height);
                delete img;
            }.bind(this);
            img.onerror = function (stuff) {
                console.log("Img Onerror:", stuff);
            };
        } else {
            if ( this._rgba === null) {
                this._canvas.width = picture.payload.wid;
                this._canvas.height = picture.payload.hei;
                this._canvasContext = this._canvas.getContext("2d");
                this._rgba = this._canvasContext.getImageData(0, 0, this._canvas.width, this._canvas.height);
            }
            
            yuv2ImageData(picture.payload.yuv, this._rgba);
            this._canvasContext.putImageData(this._rgba, 0, 0);
        }
    }.bind(this);

    this._playVideo = function() {
        if ( this._videoBufferList.length >= 2) {
            setTimeout(this._playVideo, 200);
        }

        this._showPicture(this._videoBufferList[0] );
        delete this._videoBufferList[0].payload;
        this._videoBufferList.shift();
    }.bind(this);

    this._pushVideoBuffer = function(payload, type) {
        var v = {};
        v.payload = payload;
        v.t = type;

        this._videoBufferList.push(v);

        if ( this._videoBufferList.length === 1) {
            setTimeout(this._playVideo, 10);
        }

    }.bind(this);

    // init
    this._constructor();
}
