
#include "HW_init.hpp"
#include "config_payload.h"
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

int HW_config_init(void) {
  int ret = 0;
  const std::string dev = "/dev/nv_driver_lite_config";
  const std::string driv_version = "/proc/nv_driver_lite/driver_version";
  // open the nv driver version /proc
  if (0 > (ret = open(driv_version.c_str(), O_RDONLY))) {
    return ret;
  }

  char buff[256] = {'\0'};
  // read it's version for logging
  ssize_t rRet = read(ret, buff, sizeof(buff));
  if (rRet < 0) {
    return ret;
  }
  buff[255] = '\0';
  std::cout << "NV Driver version: " << buff << std::endl;

  close(ret);

  // open the config device endpoint
  if (0 > (ret = open(dev.c_str(), O_WRONLY))) {
    return ret;
  }
  // write the magic config blob to it
  ssize_t wRet = write(ret, payload_bin, payload_bin_len);
  if (wRet < 0)
    return wRet;
  close(ret);
  return ret;
}
