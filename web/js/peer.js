function Peer(cameraName, peerName, wsURL) {
    //public var
    this.cameraName = cameraName;
    this.peerName = peerName;
    this.state = -1;           //-1 offline; 0 connecting; 1 online;

    //private var
    this._wsURL = wsURL;
    this._xml = "";
    this._conn = null;

    // callback
    this.onLogin = null;
    this.onLogout = null;
    this.onRemoteLogin = null;
    this.onRemoteLogout = null;
    this.onMessage = null;

    // public functions
    this.login = function() {
        this._conn = new  WebSocket(this._wsURL, "viewer_" + this.cameraName + "_" + this.peerName);
        this._conn.onopen = this._onOpen;
        this._conn.onmessage = this._onMessage;
        this._conn.onclose = this._onClose;
        this._xml = "";
        this.state = 0;
    }.bind(this);

    this.logout = function () {
        this._conn.close();
    }.bind(this);

    this.sendMessage = function (remote, text) {
        this._conn.send("<message:" + remote + ":" + text + ">");
    }.bind(this);

    this.doShout = function(text) {
        this._conn.send("<shout:" + text + ">");
    }.bind(this);

    // internal functions
    this._onOpen = function() {

    }.bind(this);

    this._onMessage = function(evt) {
      var msg = evt.data;
      for(var i =0; i < msg.length; i++) {
          this._xml += msg.substr(i,1);
          if ( msg.charAt(i) == '>') {
              this._processXML();
              this._xml = "";
          }
      }
    }.bind(this);

    this._onClose = function() {
      delete this._conn;
      this._conn = null;
      this._xml = null;
      this.onLogout("network");
    }.bind(this);

    this._processXML = function() {
      var wordBegin = this._xml.indexOf("<");
      var wordEnd = this._xml.indexOf(">");
      var sentence = this._xml.substring(wordBegin+1, wordEnd);
      var words = sentence.split(":");

      if ( words.length == 0)
          return;

      if ( words[0] === "login" && words.length == 2) {
          if ( words[1] == "ok" ) {
              if ( this.state == 0) {
                  this.state = 1;
                  this.onLogin(true);
              }
          } else if ( words[1] == "error" ) {
              delete this._conn;
              this._conn = null;
              this._xml = null;
              this.onLogin(false);
              return;
          }
      }

      if ( this.state <= 0)
         return;


      if ( words[0] === "online" && words.length === 2) {
          this.onRemoteLogin(words[1]);
          return;
      }

      if ( words[0] === "offline" && words.length === 2) {
          this.onRemoteLogout( words[1] );
          return;
      }

      if ( words[0] === "message" && words.length >= 3) {
          this.onMessage( words[1], words.slice(2, words.length) );
          return;
      }

      if ( words[0] === "shout" && words.length >= 3) {
          this.onMessage( words[1], words.slice(2, words.length));
          return;
      }

    }.bind(this);

    // init code

};
