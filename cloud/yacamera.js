// Global functions
String.prototype.beginWith = function(needle) {
    return(this.indexOf(needle) == 0);
};

Array.prototype.remove = function(e) {
    for (var i = 0; i < this.length; i++) {
        if (e == this[i]) { return this.splice(i, 1); }
    }
};


// Core class define
var assert = require('assert');
var config = require('./config');
var Peer = require('./peer');
function Camera(name, connection) {
    this.peer = new Peer(name, "camera", connection);
    this.viewers = {};
    this.streamingTree = [[]];
    this.streamingTree[0][0] = this.peer;
    this.peer.level = 0;

    this.peer.message("<login:ok>");

    // setup incoming camera
    this.onMessage = function(peer, words) {
        if ( words.length >= 2 && words[0] === "shout" ) {
            words.splice(1, 0, peer.name);
            var msg = this.words2Message(words);

           for(i = 0; i <  peer.up.length; i++) {
                peer.up[i].message(msg);
           }
           for(i = 0; i <  peer.down.length; i++) {
                peer.down[i].message(msg);
           }
        } else if ( words.length >= 3 && words[0] === "message" ) {
           if ( words[1] === this.peer.name ) {        // send to camera
               words[1] = peer.name;
               msg = this.words2Message(words);
               this.peer.message(msg);
            } else {
               if ( this.viewers[words[1]] !== undefined ) {
                    v = this.viewers[words[1]];
                    words[1] = peer.name;
                    msg = this.words2Message(words);
                    v.message(msg);
                }
            }
        }

    }.bind(this);

    this.onClose = function(peer) {

        if ( peer.role === "viewer") {
            this.delViewer(peer);
        } else {
            for(var p in this.viewers ) {
                this.viewers[p].message("<offline:" + this.peer.name);
                this.viewers[p].close();
                delete this.viewers[p];
            }
            delete onlineCameras[this.peer.name];
            delete this;
        }

    }.bind(this);

    this.peer.onMessage = this.onMessage;
    this.peer.onClose = this.onClose;

    this.delViewer = function(viewer) {
        if ( this.viewers[viewer.name] !== viewer) {
            return;
        }

        var selectedViewer = null;
        this.streamingTree[viewer.level].remove(viewer);

        if ( viewer.level === (this.streamingTree.length - 1)) {
            if ( this.streamingTree[viewer.level].length == 0) {
                this.streamingTree.splice(viewer.level, 1);
            }
        } else {
            if ( this.streamingTree.length > 2) {
                selectedViewer = this.streamingTree[this.streamingTree.length-1].pop();
                this.streamingTree[selectedViewer.level].remove(viewer);

                if ( this.streamingTree[selectedViewer.level].length == 0) {
                    this.streamingTree.splice(selectedViewer.level, 1);
                }
            }
        }

        if ( selectedViewer !== null) {
          assert( selectedViewer.down.length == 0, 'delViewer error!');

          viewer.down.remove(selectedViewer);
          this.streamingTree[viewer.level].push(selectedViewer);


          ups = selectedViewer.up;
          selectedViewer.up = viewer.up;
          selectedViewer.down = viewer.down;
          selectedViewer.level = viewer.level;

          viewer.up.forEach(function(v){
              v.down.remove(viewer);
              v.down.push(selectedViewer);
          });
          viewer.down.forEach(function(v){
              v.up.remove(viewer);
              v.up.push(selectedViewer);
          });

          ups.forEach(function(v){
              v.down.remove(selectedViewer);
          });

          // Send the new network info to others
          for(i = 0; i < ups.length; i++) {
              ups[i].message("<offline:" + selectedViewer.name + ">");
              selectedViewer.message("<offline:" + ups[i].name + ">");
          }
          for(i = 0; i < selectedViewer.up.length; i++) {
              selectedViewer.message("<online:" + selectedViewer.up[i].name  + ">");
              selectedViewer.up[i].message("<offline:" + viewer.name  + ">");
              selectedViewer.up[i].message("<online:" + selectedViewer.name + ">");
          }
          for(i = 0; i < selectedViewer.down.length; i++) {
              selectedViewer.message("<online:" + selectedViewer.down[i].name + ">");
              selectedViewer.down[i].message("<offline:" + viewer.name + ">");
              selectedViewer.down[i].message("<online:" + selectedViewer.name + ">");
          }

        } else {
          viewer.up.forEach(function(v){
              v.message("<offline:" + viewer.name + ">");
              v.down.remove(viewer);
          });
          viewer.down.forEach(function(v){
              v.message("<offline:" + viewer.name + ">");
              v.up.remove(viewer);
          });

        }

        // delete object
        delete this.viewers[viewer.name];
        delete viewer;

        //this.dump();
   }.bind(this);

    this.addViewer = function (name, connection) {
        var viewer = new Peer(name, "viewer", connection);
        viewer.onMessage = this.onMessage;
        viewer.onClose = this.onClose;
        this.viewers[name] = viewer;

        if ( this.streamingTree.length == 1 ) {
            viewer.up.push(this.peer);

            this.streamingTree[1] = [];
            this.streamingTree[1].push(viewer);
            viewer.level = 1;
        } else {
            // first judge this level is full?
            var isFull = false;
            var freeNodeList = [];
            var upLevel = this.streamingTree.length - 2;

            if ( this.streamingTree.length == 2 ) {
                if ( this.streamingTree[1].length == config.sourceLink) {
                    isFull = true;
                }
            } else {
                for(i = 0; i < this.streamingTree[upLevel].length; i++) {
                    if ( this.streamingTree[upLevel][i].down.length < config.downNumber ) {
                        freeNodeList.push( this.streamingTree[upLevel][i] );
                    }
                }
                if ( freeNodeList.length < config.upNumber ) {
                    isFull = true;
                }
            }

            if ( isFull ) {
               freeNodeList = [];
               upLevel = upLevel + 1;
               for(i = 0; i < this.streamingTree[upLevel].length; i++) {
                   if ( this.streamingTree[upLevel][i].down.length < config.downNumber ) {
                       freeNodeList.push( this.streamingTree[upLevel][i] );
                   }
               }
               this.streamingTree.push([]);
            }

            if ( freeNodeList.length > 0) {
                for (i = 0; (freeNodeList.length > 0) && (i < config.upNumber); i++) {
                    var up = Math.floor( freeNodeList.length * Math.random() );
                    viewer.up.push( freeNodeList[up] );
                    freeNodeList.splice(up, 1);
                }
            } else {
                // only level 2
                assert( this.streamingTree.length == 2 && this.streamingTree[1].length < config.sourceLink , " Createing streamingTree error! ");
                viewer.up.push(this.peer);
            }

            this.streamingTree[this.streamingTree.length-1].push(viewer);
            viewer.level = this.streamingTree.length -1;
        }

        for(i = 0; i < viewer.up.length; i++) {
            viewer.up[i].down.push(viewer);
        }

        // send the linked peer to new online viewer
        viewer.message("<login:ok>");
        var msg = "<online:" + viewer.name + ">";
        for(i = 0; i < viewer.up.length; i++) {
            viewer.up[i].message("<online:" + viewer.name + ">");
            viewer.message("<online:" + viewer.up[i].name + ">");
        }
        for(i = 0; i < viewer.down.length; i++) {
            viewer.down[i].message("<online:" + viewer.name + ">");
            viewer.message("<online:" + viewer.down[i].name + ">");
        }

        //this.dump();
    }.bind(this);


    this.words2Message = function(words) {
        var msg = "<";
        for(i = 0; i < words.length - 1; i++) {
            msg = msg + words[i] + ":";
        }
        msg = msg + words[words.length-1] + ">";
        return msg;
    };

    this.dump = function() {
        console.log("----------------------------")
        for(i = 0; i < this.streamingTree.length; i++) {
            var levelInfo = "L" + i +  "(" +  this.streamingTree[i].length  + "):";
            for (j = 0; j < this.streamingTree[i].length; j++) {
                levelInfo += "[" + this.streamingTree[i][j].name ;
                for(k = 0; k < this.streamingTree[i][j].up.length; k++) {
                  levelInfo += ":" + this.streamingTree[i][j].up[k].name ;
                }
                levelInfo += "|";
                for(k = 0; k < this.streamingTree[i][j].down.length; k++) {
                  levelInfo += ":" + this.streamingTree[i][j].down[k].name ;
                }
                levelInfo += "] ";
            }
            console.log(levelInfo);
        }
    }.bind(this);
};

// Gloabal data
var onlineCameras = {};

var newConnection = function(targetProtocol, connection) {
    var argvs = targetProtocol.split("_");
    if (argvs.length === 2 && argvs[0] === "camera") {
        if ( onlineCameras[argvs[1]] === undefined ) {
            var newCamera = new Camera(argvs[1], connection);
            onlineCameras[argvs[1]] = newCamera;
            return;
        }
    } else if ( argvs.length === 3 && argvs[0] === "viewer" && argvs[2] !== "") {
        if ( onlineCameras[argvs[1]] !== undefined ) {
            var cam = onlineCameras[argvs[1]];
            if ( cam.viewers[ argvs[2] ] === undefined) {
                cam.addViewer(argvs[2], connection);
                return;
            }
        }
    }
    connection.write("<login:error>");
    connection.close();
};

var httpModule = require("http");
var urlModule = require('url');
httpModule.createServer(function(request, response) {
    var url = urlModule.parse(request.url, true);
    if ( url.pathname === "/admin/online" ) {
        response.writeHead(200, {"Content-Type": "text/html"});
        response.write("Online cameras:<br>");
        for(var p in  onlineCameras) {
            response.write(onlineCameras[p].peer.name + ":" + Object.keys(onlineCameras[p].viewers).length + "<br>");
        }
        response.end();
    } else {
        response.end();
    }
}).listen(config.adminPort);

// Exported functions
module.exports.newConnection = newConnection;
