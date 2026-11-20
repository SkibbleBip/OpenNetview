#ifndef __WEBCONNECT_HPP__
#define __WEBCONNECT_HPP__

#include "Common.h"
#include "VideoManager.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

enum COMMAND_TYPE { NOOP = 0, MOVE = 1, HOME = 2 };

class WebConnect {
private:
  int command_socket = -1;
  int m_error = 0;
  VideoManager *videoManager = nullptr;

public:
  WebConnect(const std::string ip_addr);
  ~WebConnect(void);
  AVFrame *fetchFrame(void);
  const int getError(void) { return m_error; }

  template <typename... Args>
  int sendCommand(COMMAND_TYPE command_type, Args... args);
};

#include "WebConnect.tpp"

#endif // __WEBCONNECT_HPP__
