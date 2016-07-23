var data = {};
data.yaCamera = null;
data.isReady = false;
data.cameraCode = null;

var ui = {};
ui.onConnect = function() {
    if (data.yaCamera !== null) {
        ui.resetConnect();
        return;
    }

    data.cameraCode = $("#inputCameraCode").val();
    ui.doConnect();

    $("#btnConnect").prop('disabled', true);
    $("#btnConnect").html("Disconnect");
    $("#spanInfo").html("Connecting to server...");
};

ui.doConnect = function() {
    data.yaCamera = new YaCamera(data.cameraCode, document.getElementById("videoPlayer"));
    data.yaCamera.onConnect = function(isOK) {
        if ( isOK === false) {
            data.yaCamera.onDisconnect = null;
            alert("Wrong camera code!");
            ui.resetConnect();
        } else {
            $("#btnConnect").prop('disabled', false);
            $("#btnConnect").html("Disconnect");
            $("#spanInfo").html("Connected, waiting for media...");
        }
    };
    data.yaCamera.onDisconnect = function() {
        alert("Disconneced from server!");
        ui.resetConnect();
    };
    data.yaCamera.onStream = function(isOK) {
        if ( data.isReady === false && isOK === true) {
            data.isReady = true;
            var url = "http://yacamera.com/?" + data.cameraCode;
            var infoStr = 'Media is ready, share with <a href="' + url + '">';
            infoStr = infoStr + url + '</a>';
            $("#spanInfo").html(infoStr);
        }
    }
}

ui.resetConnect = function() {
    window.location.replace("http://yacamera.com");
};

// 全局初始化函数
$(document).ready(function() {
    if ( RTCPeerConnection !== null && webrtcDetectedBrowser !== null && webrtcDetectedVersion !== null) {
        $("#btnConnect").click(ui.onConnect);

        var cameraCode = $(location).attr('search').slice(1);
        if ( cameraCode.length > 0) {
            $("#inputCameraCode").val(cameraCode);
            ui.onConnect();
        }
    } else {
        $("#btnConnect").attr("disabled", "disabled");
        alert("YaCamera only support Chrome with WebRTC!");
    }
});
