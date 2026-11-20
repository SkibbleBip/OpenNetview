

#include "WebConnect.hpp"

#include <errno.h>
#include <iostream>
#include <netinet/in.h>
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

  // set up FFMPEG to listen on h264 port
  videoManager = new VideoManager(H264_PORT);

  if (videoManager->getError()) {
    m_error = EBADMSG;
    std::cerr << "Failed to set up FFMPEG" << std::endl;
    return;
  }

  std::cout << "Set up h264 FFMPEG connection" << std::endl;
}

WebConnect::~WebConnect(void) {
  if (command_socket >= 0) {
    close(command_socket);
  }

  delete videoManager;
  //  if (h264_connection >= 0) {
  //    close(h264_connection);
  //  }
}

AVFrame *WebConnect::fetchFrame(void) {
  return videoManager->fetchFrame();
  /// @todo multithreaded fetching;
}
