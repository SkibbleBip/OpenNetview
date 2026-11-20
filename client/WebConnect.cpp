

#include "WebConnect.hpp"

#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <new>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

WebConnect::WebConnect(const std::string ip_addr) {

  if ((command_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    command_socket = -1;
    // h264_connection = -1;
    m_error = errno;
    std::cerr << "Failed to init command socket: " << m_error << std::endl;
    return;
  }

  struct sockaddr_in serv_addr = {0};

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(COMMAND_PORT);
  int ret = 0;
  if ((ret = inet_pton(AF_INET, ip_addr.c_str(), &serv_addr.sin_addr)) <= 0) {
    // h264_connection = -1;
    m_error = (ret == 0 ? EINVAL : errno);
    std::cerr << "Failed to convert IP Addr: " << m_error << std::endl;
    return;
  }

  if (0 != connect(command_socket, (struct sockaddr *)&serv_addr,
                   sizeof(serv_addr))) {
    // h264_connection = -1;
    m_error = errno;
    std::cerr << "Failed to connect to socket: " << m_error << std::endl;
    return;
  }
  std::cout << "Connected to Camera CNC: " << ip_addr << ":" << COMMAND_PORT
            << std::endl;

  m_rwlock = PTHREAD_RWLOCK_INITIALIZER;
  // init the rw lock

  // set up FFMPEG to listen on h264 port
  sharedMem.isRunning.store(true);
  sharedMem.rw_lock = &m_rwlock;
  sharedMem.portNum = H264_PORT;
  sharedMem.frameCache = nullptr;
  ret = pthread_create(&m_video_thread, nullptr, mf_video_thread,
                       static_cast<void *>(&sharedMem));
  if (ret < 0) {
    m_error = errno;
    std::cerr << "Failed to create video thread: " << m_error << std::endl;
  }
}

WebConnect::~WebConnect(void) {
  if (command_socket >= 0) {
    close(command_socket);
  }
  sharedMem.isRunning.store(false);

  pthread_join(m_video_thread, nullptr);

  pthread_rwlock_destroy(&m_rwlock);
  av_frame_free(&(sharedMem.frameCache));
}

void *WebConnect::mf_video_thread(void *ptr) {
  struct shared_mem *sharedMem = static_cast<struct shared_mem *>(ptr);
  pthread_rwlock_t *l_rw_lock = sharedMem->rw_lock;
  const unsigned short portNum = sharedMem->portNum;

  VideoManager videoManager(portNum);

  int ret = 0;
  if (ret = videoManager.getError()) {
    std::cerr << "Failed to set up FFMPEG: " << ret << std::endl;
    return nullptr;
  }

  std::cout << "Set up h264 FFMPEG connection" << std::endl;

  while (sharedMem->isRunning == true) {
    AVFrame *framePtr = videoManager.fetchFrame();
    int lock_ret = pthread_rwlock_trywrlock(l_rw_lock);
    if (lock_ret == EBUSY) {
      continue;
      // if failed to lock because we are reading, then
      // just skip
    } else if (0 > lock_ret) {
      int err = errno;
      std::cerr << "Failed to write-lock mutex: " << err << std::endl;
      return nullptr;
    }
    av_frame_free(&(sharedMem->frameCache));
    sharedMem->frameCache = av_frame_clone(framePtr);
    if (0 > pthread_rwlock_unlock(l_rw_lock)) {
      int err = errno;
      std::cerr << "Failed to unlock mutex: " << err << std::endl;
      return nullptr;
    }
  }
  av_frame_free(&(sharedMem->frameCache));
  return nullptr;
}

AVFrame *WebConnect::fetchFrame(void) {
  if (0 > pthread_rwlock_rdlock(&m_rwlock)) {
    int err = errno;
    std::cerr << "Failed to read-lock mutex: " << err << std::endl;
    return nullptr;
  }
  return sharedMem.frameCache;
}

void WebConnect::releaseFrame(void) {
  if (0 > pthread_rwlock_unlock(&m_rwlock)) {
    int err = errno;
    std::cerr << "Failed to read-lock mutex: " << err << std::endl;
  }
}
