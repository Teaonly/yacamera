// require core application files
var config = require('./config');
var server = require('./server');
var yaCamera = require('./yacamera');

// start the application
server.start(config, yaCamera);
