#include "Frame.hpp"
#include <iostream>

Frame getFrame(void) {

  void *dumbPtr;
  int ret = IMP_FrameSource_GetFrame(0, &dumbPtr);
  if (ret != 0) {
    std::cerr << "Failed to get frame: " << ret << std::endl;
    return Frame(ret);
  }

  // because of type missmatch, we need to convert the raw pointer
  // over to a useable struct
  custom_IMPFrameInfo *frame = reinterpret_cast<custom_IMPFrameInfo *>(dumbPtr);

  return Frame(frame);
}

int freeFrame(Frame frame) {
  int ret = IMP_FrameSource_ReleaseFrame(
      0, reinterpret_cast<void *>(frame.getFrameInfo()));
  if (ret != 0) {
    std::cerr << "Failed to release frame: " << ret << std::endl;
  }

  return ret;
}
