#ifndef _MEDIABUFFER_H_
#define _MEDIABUFFER_H_

#include <list>
#include <vector>
#include "webrtc/base/criticalsection.h"

const int MUX_OFFSET = 0;

struct MediaPackage {
public:
  MediaPackage(const int size) {
	data_ = new unsigned char[size + MUX_OFFSET];
    data_ += MUX_OFFSET;
  };
  ~MediaPackage() {
    if ( data != NULL) {
      data_ = data_ - MUX_OFFSET;
      delete data_;
    }
  }
  
  unsigned char *data_;
  unsigned char *data;
  unsigned int length;
};

class MediaBuffer {
public:
  MediaBuffer(const unsigned int num, const unsigned int size);
  ~MediaBuffer();
  void Reset();

  // access from diffrent threads, they are safe.
  bool PushBuffer(const unsigned char *d, const unsigned int len);
  bool PullBuffer();
  MediaPackage *Released();

  unsigned int BufferSize(){
    return buffer_.size();
  }

  unsigned int FreeSize() {
    return pkg_pool_.size();
  }

private:
  unsigned int pkg_size_;
  
  std::list<MediaPackage*> buffer_;
  std::vector<MediaPackage*> pkg_pool_;
  MediaPackage *pkg_released;
  
  rtc::CriticalSection mutex_; 
};

#endif
