function Peer(name, role, connection) {
    this.name = name;
    this.role = role;
    this._connection = connection;
    this._xmlWord = "";
    
    this.level = -1;
    this.up = [];
    this.down = [];

    this._processXML = function() {
        var wordBegin = this._xmlWord.indexOf("<");
        var wordEnd = this._xmlWord.indexOf(">");
        var sentence = this._xmlWord.substring(wordBegin+1, wordEnd);
        var words = sentence.split(":");
        if ( this.onMessage !== null) {
            this.onMessage(this, words);
        }
    }.bind(this);
    this._onMessage = function(msg) {
        var d = msg;
        if ( msg.utf8Data !== undefined ) {
            d = msg.utf8Data;
        }
        for(var i = 0; i < d.length; i++) {
            this._xmlWord += d.substr(i,1);

            if(d.charAt(i) == '>') {
                this._processXML();
                this._xmlWord = "";
            }
        };
    }.bind(this);

    // setup connection's event handler
    this._onClose = function(reasonCode, description) {
        if ( this.onClose !== null) {
            this.onClose(this);
        }
    }.bind(this);

    this.message = function(msg) {
        try {
            this._connection.write(msg);
        } catch (e) {
        }
    }.bind(this);

    this.close = function(msg) {
        this._connection.close();
    }.bind(this);

    this._connection.on('message', this._onMessage);
    this._connection.on('close', this._onClose);


    //  callbacks
    this.onMessage  = null;
    this.onClose = null;
};

module.exports = Peer;
