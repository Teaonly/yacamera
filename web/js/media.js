var yaParser = {};
yaParser.mediaType = -1;     // 0: jpeg; 1: h.264
yaParser.mediaSequence = -1;
yaParser.K = -1;
yaParser.esi = -1;
yaParser.mediaLength = -1;
yaParser.pictureLength = -1;
yaParser.packet = null;

yaParser.newPacket = function(txtPacket) {
    this.mediaType = -1;
    this.mediaSequence = -1;
    this.K = -1;
    this.esi = -1;
    this.mediaLength = -1;
    this.pictureLength = -1;

    packet = Base64.binDecode(txtPacket);
    var header = (packet[1] << 8) + packet[0];
    // packet's first four bytes should be 0x1979 (6521)
    if ( header === 6521)  {
        this.mediaType = 0;
    } else if ( header === 6528) {
        this.mediaType = 1;
    } else {
        return false;
    }

    this.mediaSequence = (packet[3] << 8) + packet[2];
    this.K =  (packet[5] << 8) + packet[4];
    this.esi = (packet[7]<< 8) + packet[6];
    this.mediaLength = (packet[11] << 24) + (packet[10] << 16) + (packet[9] << 8) + packet[8];
    this.pictureLength = (packet[15] << 24) + (packet[14] << 16) + (packet[13] << 8) + packet[12];

    this.packet = packet.slice(16);

    return true;
}.bind(yaParser);


function YaMedia(mediaType, seq, K, mediaLength, pictureLength) {
    this.mediaType = mediaType;
    this.mediaSequence = seq;
    this.K = K;
    this.mediaLength = mediaLength;
    this.pictureLength = pictureLength;
    this.raptor = new Raptor(K, this.mediaLength);
    this.symbolNumber = 0;
    this.isReady = false;

    this.release = function() {
        this.raptor.release();
        delete this.inputBuffer;
    }.bind(this);

    this.pushSymbol = function(esi, block) {
        this.symbolNumber ++;
        this.raptor.pushSymbol(esi, block);
    }.bind(this);

    this.doDecode = function() {
        if ( this.raptor.doDecode() === false) {
            return false;
        }
        if ( this.raptor.buildPayload() === false) {
            return false;
        }
        targetMedia.isReady = true;
        return true;
    }.bind(this);

    this.getPicturePayload = function() {
        var payload = this.raptor.getPayload(0, this.pictureLength);
        return payload;
    }.bind(this);

    this.getAudioPayload = function() {
        var payload = this.raptor.getPayload(this.pictureLength, this.mediaLength - this.pictureLength);
        return payload;
    }.bind(this);

    this.buildPacket = function(esi) {
        var payload = this.raptor.getSymbol(esi);
        var packet = new Uint8Array(16 + payload.length);
        for(i=0; i < payload.length; i++) {
            packet[i+16] = payload[i];
        }
        delete payload;

        if ( this.mediaType === 0) {
            packet[0] = 0x79;
        } else if (this.mediaType === 1) {
            packet[0] = 0x80;
        }
        packet[1] = 0x19;
        packet[2] = (this.mediaSequence & 0xFF);
        packet[3] = (this.mediaSequence & 0xFF00) >> 8;

        packet[4] = (this.K & 0xFF);
        packet[5] = (this.K & 0xFF00) >> 8;

        packet[6] = (esi & 0xFF);
        packet[7] = (esi & 0xFF00) >> 8;

        packet[8] = (this.mediaLength & 0xFF);
        packet[9] = (this.mediaLength & 0xFF00) >> 8;
        packet[10] = (this.mediaLength & 0xFF0000) >> 16;
        packet[11] = (this.mediaLength & 0xFF000000) >> 24;

        packet[12] = (this.pictureLength & 0xFF);
        packet[13] = (this.pictureLength & 0xFF00) >> 8;
        packet[14] = (this.pictureLength & 0xFF0000) >> 16;
        packet[15] = (this.pictureLength & 0xFF000000) >> 24;

        var txtPacket =  btoa(String.fromCharCode.apply(null, packet));
        return txtPacket;
    }.bind(this);
};
