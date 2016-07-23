// require third party libraries. 
var http = require('http');
var WebSocketServer = require('websocket').server;
var WebSocketConnection = require('websocket').connection;
WebSocketConnection.prototype.write = function(msg) {
    this.sendUTF(msg);
};

var server = {};
server.start = function(config, appCore) {   

    // Create websocket server
    var server = http.createServer(function(request, response) {
        response.end();
    }).listen(config.wsServerPort, function() {
        
    });
    var wsServer = new WebSocketServer({
        httpServer: server,
        autoAcceptConnections: false
    });
    wsServer.on('request', function(request) {
        var targetProtocol = request.requestedProtocols[0];
        if ( targetProtocol === undefined ) {
            targetProtocol = request.resource.replace("/ws/", "");
        }
        var connection = request.accept(request.requestedProtocols[0], request.origin);   
        appCore.newConnection(targetProtocol, connection); 
    });

    // Create TCP server just for debug
    var net = require("net");
    net.createServer(function(sock) {

        sock.setTimeout(0);
        sock.setEncoding("utf8");
        sock.close = sock.end;
        
        sock.on = function(evt, handler) {
            if ( evt === 'close' ) {
                this.addListener("error", handler);
                this.addListener("close", handler);
            } else if ( evt === "message") {
                this.addListener("data", handler);
            }
        }.bind(sock);
        
        var protocol = "viewer_debug_v" + Math.floor(Math.random()*1000);
                
        appCore.newConnection(protocol, sock); 
            
    }).listen(config.debugPort); 
};

module.exports = server;
