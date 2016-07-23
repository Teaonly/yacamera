#
#   Building script for ipcamera application
#
{
    'includes': ['build/common.gypi'],
    'targets' : [
    {
        'target_name': 'camerartc',
        'type': 'executable',
        'include_dirs': [ 
            '../third_party/webrtc/modules/interface',
        ], 
        'dependencies': [
            'libjingle.gyp:libjingle_peerconnection',
            '<(DEPTH)/third_party/jsoncpp/jsoncpp.gyp:jsoncpp',
        ],
        'sources': [
            'camerartc/main.cpp',
            'camerartc/peer.h',
            'camerartc/peer.cpp',
            'camerartc/camerartc.h',
            'camerartc/camerartc.cpp',
            'camerartc/rtcstream.h',
            'camerartc/rtcstream.cpp',
            'camerartc/attendee.h',
            'camerartc/attendee.cpp',
            'camerartc/mediabuffer.h',
            'camerartc/mediabuffer.cpp',
            'camerartc/raptor/numberdb.h',
            'camerartc/raptor/numberdb.cpp',
            'camerartc/raptor/rprandom.h',
            'camerartc/raptor/rprandom.cpp', 
            'camerartc/raptor/raptor.h',
            'camerartc/raptor/aaptor.cpp',
            'camerartc/g72x/g726_32.c',
            'camerartc/g72x/g711.c',
            'camerartc/g72x/g72x.c',
            ],
    },
    ],
}
