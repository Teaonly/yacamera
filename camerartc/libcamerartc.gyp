#
#   Building script for ipcamera application
#
{
  'includes': ['build/common.gypi'],
  'conditions': [
    [ 'OS == "android"', {
      'targets': [
        {
          'target_name': 'libjingle_camerartc_so',
          'type': 'loadable_module',
          'dependencies': [
            'libjingle.gyp:libjingle_peerconnection',
            '<(DEPTH)/third_party/jsoncpp/jsoncpp.gyp:jsoncpp',
          ],
          'sources': [
            'camerartc/main_jni.cpp',
            'camerartc/camerartc.h',
            'camerartc/camerartc.cpp',
            'camerartc/rtcstream.h',                
            'camerartc/rtcstream.cpp',                
            'camerartc/peer.h',
            'camerartc/peer.cpp',
            'camerartc/jnipeer.h',
            'camerartc/jnipeer.cpp',
            'camerartc/attendee.h',                
            'camerartc/attendee.cpp',                
            'camerartc/mediabuffer.h',                
            'camerartc/mediabuffer.cpp',                
            'camerartc/raptor/rprandom.h',
            'camerartc/raptor/rprandom.cpp',
            'camerartc/raptor/numberdb.h',
            'camerartc/raptor/numberdb.cpp',
            'camerartc/raptor/raptor.h',
            'camerartc/raptor/raptor.cpp',
            'camerartc/g72x/g726_32.c',
            'camerartc/g72x/g711.c',
            'camerartc/g72x/g72x.c',
            ],
        },
      ],
    }],
  ],
}
