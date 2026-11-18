#ifndef __WEBCONNECT_HPP__
#define __WEBCONNECT_HPP__

#include "Common.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

typedef std::unique_ptr<uint8_t> H264_FRAME;

enum COMMAND_TYPE { NOOP = 0, MOVE = 1, HOME = 2 };

class WebConnect {
private:
  int h264_connection = -1;
  int command_socket = -1;
  int m_error = 0;

public:
  WebConnect(const std::string ip_addr);
  ~WebConnect(void);
  H264_FRAME fetchFrame(void);
  const int getError(void) { return m_error; }

  template <typename... Args>
  int sendCommand(COMMAND_TYPE command_type, Args... args);
};

#include "WebConnect.tpp"

#endif // __WEBCONNECT_HPP__
