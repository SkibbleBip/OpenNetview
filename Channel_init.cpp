#include "Channel_init.hpp"

#include "imp/imp_framesource.h"
#include "imp/imp_isp.h"
#include <cstddef>
#include <cstdint>
#include <iostream>

int Channel_init(int channelID, uint32_t width, uint32_t height) {
  // we need the sensor framerate first
  int ret = 0;
  uint32_t fps_num = 0, fps_den = 0;
  ret = IMP_ISP_Tuning_GetSensorFPS(&fps_num, &fps_den);
  if (ret != 0) {
    std::cerr << "Failed to get sensor tuning FPS: " << ret << std::endl;
    return ret;
  }

  std::cout << "NV framerate: " << fps_num << "/" << fps_den << std::endl;

  IMPFSChnAttr channelAttr = {0};
  channelAttr.picWidth = width;
  channelAttr.picHeight = height;
  channelAttr.pixFmt = PIX_FMT_NV12;
  channelAttr.outFrmRateNum = fps_num;
  channelAttr.outFrmRateDen = fps_den;
  channelAttr.nrVBs = 1;
  channelAttr.type = FS_PHY_CHANNEL;
  // set frame format and framerate

  channelAttr.crop = {0};   // completely disable cropping
  channelAttr.scaler = {0}; // completely disable scalar

  ret = IMP_FrameSource_CreateChn(channelID, &channelAttr);
  if (ret != 0) {
    std::cerr << "Failed to create channel 0: " << ret << std::endl;
    return ret;
  }

  ret = IMP_FrameSource_EnableChn(channelID);
  if (ret != 0) {
    std::cerr << "Failed to enable channel " << channelID << ": " << ret
              << std::endl;
    return ret;
  }
  std::cout << "Sucessfully enabled channel " << channelID << std::endl;

  // pre-empt the depth of frames
  ret = IMP_FrameSource_SetFrameDepth(channelID, 3);
  if (ret != 0) {
    std::cerr << "Failed to set frame depth: " << ret << std::endl;
    return ret;
  }

  return ret;
}

int Channel_close(int channelID) {
  int ret = ret = IMP_FrameSource_DisableChn(channelID);
  // disable the channel
  if (ret < 0) {
    std::cerr << "Failed to disable channel " << channelID << ": " << ret
              << std::endl;
    return ret;
  }
  ret = IMP_FrameSource_DestroyChn(channelID);
  if (ret < 0) {
    std::cerr << "Failed to destroy channel " << channelID << ": " << ret
              << std::endl;
    return ret;
  }
  return 0;
}
