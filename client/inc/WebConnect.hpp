#ifndef __WEBCONNECT_HPP__
#define __WEBCONNECT_HPP__

#include "Common.h"
#include "VideoManager.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

enum COMMAND_TYPE { NOOP = 0, MOVE = 1, HOME = 2 };

class WebConnect {
private:
  int command_socket = -1;
  int m_error = 0;
  pthread_t m_video_thread = {};
  pthread_rwlock_t m_rwlock = {};

  struct shared_mem {
    std::atomic<bool> isRunning{false};
    pthread_rwlock_t *rw_lock = nullptr;
    unsigned short portNum;
    AVFrame *frameCache = nullptr;
  };
  struct shared_mem sharedMem = {0};

public:
  WebConnect(const std::string ip_addr);
  ~WebConnect(void);
  AVFrame *fetchFrame(void);
  void releaseFrame(void);
  const int getError(void) { return m_error; }

  template <typename... Args>
  int sendCommand(COMMAND_TYPE command_type, Args... args);
  static void *mf_video_thread(void *ptr);
};

#include "WebConnect.tpp"

#endif // __WEBCONNECT_HPP__
