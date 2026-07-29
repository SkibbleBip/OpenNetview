#include "stubs.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "Encoder.hpp"
#include "Frame.hpp"
#include "HW_init.hpp"

#include "imp/imp_system.h"

int main(int argc, char **argv) {
  std::ios_base::sync_with_stdio(false);
  // fix for broken stdio in the uClib provided by the camera's sysroot

  std::cout << "Starting OpenNetview camera" << std::endl;

  // write the magic config payload to the driver to init the hardware
  int ret = HW_config_init();
  if (ret < 0) {
    std::cerr << "Failed to init the hardware driver: " << ret << std::endl;
    return -1;
  }

  Encoder encoder(0);

  IMPVersion versionInfo = {'\0'};
  ret = IMP_System_GetVersion(&versionInfo);
  if (ret < 0) {
    std::cerr << "Failed to get IMP version" << std::endl;
    return -1;
  }
  std::cout << "IMP Version: " << versionInfo.aVersion << std::endl;

  FILE *out = fopen("output.h264", "wb");
  for (int j = 0; j < 30; j++)
    if (encoder.getError() == 0) {
      // do stuff
      std::cout << "it worked" << std::endl;

      StreamReply reply = encoder.getStream();

      // std::string path = "test" + std::to_string(j) + ".h264";
      // FILE *out = fopen(path.c_str(), "wb");

      for (int idx = 0; idx < reply.streamData.packCount; idx++) {
        custom_IMPEncoderPack *pack = &reply.streamData.pack[idx];
        /*printf("physical %p virtual %p length %d timestamp %ld frameEnd %d
        datatype %d\n", pack->phyAddr, pack->virAddr, pack->length,
        pack->timestamp, pack->frameEnd, pack->dataType
        );*/

        fwrite((void *)pack->virAddr, 1, pack->length, out);
      }

      encoder.releaseStream(&reply);
    }

  fclose(out);

  /*Frame frame = getFrame();


  if(frame.getError()){
          std::cerr << "Failed to get frame: " << frame.getError() << std::endl;
  }
  else{
          custom_IMPFrameInfo* frameInfo = frame.getFrameInfo();
          std::ofstream out("Frame.data", std::ios::binary);

          out.write(reinterpret_cast<char *>(frameInfo->virAddr),
  frameInfo->size);
  }

  freeFrame(frame);*/

  return 0;
}
