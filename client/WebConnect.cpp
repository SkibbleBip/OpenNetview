

#include "WebConnect.hpp"

#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

WebConnect::WebConnect(const std::string ip_addr) {

  if ((command_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    command_socket = -1;
    h264_connection = -1;
    m_error = errno;
    std::cerr << "Failed to init command socket: " << m_error << std::endl;
    return;
  }

  struct sockaddr_in serv_addr = {0};

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(COMMAND_PORT);
  int ret = 0;
  if ( (ret = inet_pton(AF_INET, ip_addr.c_str(), &serv_addr.sin_addr)) <= 0) {
    h264_connection = -1;
    m_error = (ret == 0 ? EINVAL : errno);
    std::cerr << "Failed to convert IP Addr: " << m_error << std::endl;
    return;
  }

  if (0 != connect(command_socket, (struct sockaddr *)&serv_addr,
                   sizeof(serv_addr))) {
    h264_connection = -1;
    m_error = errno;
    std::cerr << "Failed to connect to socket: " << m_error << std::endl;
    return;
  }
  std::cout << "Connected to Camera CNC: " << ip_addr << ":" << COMMAND_PORT
            << std::endl;

  // listen for h264 frames
  if (0 > (h264_connection = socket(AF_INET, SOCK_DGRAM, 0))) {
    m_error = errno;
    std::cerr << "Failed to create h264 socket: " << m_error << std::endl;
    return;
  }

  struct sockaddr_in h264_addr = {0};
  h264_addr.sin_family = AF_INET;
  h264_addr.sin_addr.s_addr = INADDR_ANY;
  h264_addr.sin_port = htons(H264_PORT);

  if (bind(h264_connection, (const sockaddr *)&h264_addr, sizeof(h264_addr)) <
      0) {
    m_error = errno;
    std::cerr << "Failed to bind h264: " << m_error << std::endl;
    return;
  }
  std::cout << "Opened UDP H264 port " << H264_PORT << std::endl;
}

WebConnect::~WebConnect(void) {
  if (command_socket >= 0) {
    close(command_socket);
  }
  if (h264_connection >= 0) {
    close(h264_connection);
  }
}
