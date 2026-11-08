#include "stubs.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "Encoder.hpp"
#include "Frame.hpp"
#include "HW_init.hpp"
#include "Webserver.hpp"

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

  CommandServer command_server;

  if (command_server.getError() != 0) {
    std::cerr << "Failed to set up Command server: "
              << command_server.getError() << std::endl;
    return 1;
  }

  do {
    std::cout << "Waiting for connection..." << std::endl;
    struct in_addr my_addr = command_server.waitUntilConnected();
    char ip_str[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(my_addr), ip_str, INET_ADDRSTRLEN);
    std::cout << "Connected to user " << ip_str << std::endl;

    ret = command_server.getError();
    if (ret != 1) {
      std::cerr << "it broke" << std::endl;
      return 1;
    }

    VideoStream vidStream(H264_PORT, my_addr);

    while (command_server.isConnected()) {
      if (encoder.getError() == 0) {
        // do stuff

        StreamReply reply = encoder.getStream();

        for (int idx = 0; idx < reply.streamData.packCount; idx++) {
          custom_IMPEncoderPack *pack = &reply.streamData.pack[idx];

          ret = vidStream.sendPacket(pack);
        }

        encoder.releaseStream(&reply);
      } else {
        std::cerr << "Encoder failed: " << encoder.getError() << std::endl;
        break;
      }
    } // while connected
    std::cout << "No longer connected" << std::endl;

    ret = command_server.getError();
    if (ret != 0) {
      std::cerr << "Command server failed with the error: " << ret << std::endl;
      break;
    }

  } while (true);

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
