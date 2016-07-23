function Raptor(K, payloadLength) {
    this.K = K;
    this.symbols = {};
    this.isReady = false;
    this.payloadLength = payloadLength;
    this._obj = Module._initRaptor(this.K);  
    
    this.inputBuffer = new Uint8Array(2048);
    this.inputMemory = Module._malloc(2048); 
    this.outputMemory = Module._malloc(this.payloadLength + 2048); 

    this.symbolsNumber = function() {
        return Object.keys(this.symbols).length;
    }.bind(this);

    this.release = function() {
        Module._releaseRaptor(this._obj);
        this._obj = null;
        Module._free(this.outputMemory);
        Module._free(this.inputMemory);
    }.bind(this);

    this.doDecode = function() {
        if ( this.isReady === true)
            return true;

        var ret = Module._doDecode(this._obj);
        if ( ret >= 0) {
            this.isReady = true;
            return true;
        }
        
        return false;
    }.bind(this);

    this.pushSymbol = function(esi, block) {
        if ( this.isReady === true) {
            return;
        }
        this.inputBuffer.set(block);
        Module.HEAPU8.set(this.inputBuffer, this.inputMemory);

       if ( this.symbols[esi] === undefined ) {
            this.symbols[esi] = true;
            Module._pushSymbol(this._obj, esi, this.inputMemory, block.length);
        }

    }.bind(this);

    this.getSymbol = function(esi) {
        var ret = Module._getSymbol(this._obj, esi, this.inputMemory);
        if ( ret === -1)
           return null;

        var block = Module.HEAPU8.subarray(this.inputMemory, this.inputMemory + ret);
        return block;
    }.bind(this);

    this.buildPayload = function() {
        if ( this.isReady === false ) {
            return flase;
        }
        var ret = Module._buildPayload(this._obj, this.outputMemory);
        if ( ret >= 0) {
            return true;
        }
        return false;
    }.bind(this);

    this.getPayload = function(offset, length) {
        return Module.HEAPU8.subarray(this.outputMemory+offset, this.outputMemory + offset + length);
    }.bind(this);

};
