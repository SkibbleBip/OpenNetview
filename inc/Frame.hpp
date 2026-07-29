#ifndef __FRAME_HPP__
#define __FRAME_HPP__

#include "NV_helper.hpp"
#include "imp/imp_framesource.h"
#include <iostream>
#include <string>

typedef struct {
  int index;
  int pool_idx;
  uint32_t width;
  uint32_t height;
  uint32_t pixfmt;
  uint32_t size;
  uint32_t phyAddr;
  uint32_t virAddr;
  int64_t timeStamp;
  uint32_t priv[0];
} custom_IMPFrameInfo;
/// @todo OpenIMP has incomplete IMPFrameInfo definition, I have to
/// declare my own

/*class Frame{
private:
        custom_IMPFrameInfo *payload = nullptr;
        int channel = 0;

public:
        const custom_IMPFrameInfo* getPayload(void) const {
                return payload;
        }
        Frame(void) = delete;
        Frame(int channelId, custom_IMPFrameInfo* rawFrame) :
channel(channelId){ payload = rawFrame;
        }
        ~Frame(void){
                if (payload){
                        std::cout << "destroy literally everything happy in this
world" << std::endl; IMP_FrameSource_ReleaseFrame(channel, payload); std::cout
<< "done with destroying" << std::endl;
                }
        }
        Frame(const Frame& cpy) = delete;

        // move operator
        Frame(Frame&& cpy) noexcept{
                channel = cpy.channel;
                payload = cpy.payload;
                cpy.payload = nullptr;
        }


};*/

class Frame {
private:
  int errorCode = 0;
  custom_IMPFrameInfo *frameInfo = nullptr;

public:
  int getError(void) { return errorCode; }
  custom_IMPFrameInfo *getFrameInfo(void) { return frameInfo; }

  Frame(int errCode) : errorCode(errCode) {}
  Frame(custom_IMPFrameInfo *fInfo) : frameInfo(fInfo){};
  Frame(void) = delete;
};

Frame getFrame(void);
int freeFrame(Frame frame);

#endif // __FRAME_HPP__
