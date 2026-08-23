#ifndef __WEBSERVER_HPP__
#define __WEBSERVER_HPP__

#include "Command.hpp"
#include "Encoder.hpp"

#include <netinet/in.h>
#include <pthread.h>

#define H264_PORT 6969
#define COMMAND_PORT 52000

struct RTPHeader {
  uint8_t vpxcc;
  uint8_t mpt;
  uint16_t sequence;
  uint32_t timestamp;
  uint32_t ssrc;
} __attribute__((packed));

class VideoStream {
private:
  int udpPort = 0;
  struct in_addr clientAddr = {0};
  int socket_fd = 0;
  int errorVal = 0;
  RTPHeader header = {0};
  uint8_t udp_buff[1200] = {0};
  uint16_t m_sequence = 0;
  uint32_t m_timestamp = 0;

public:
  VideoStream(int port, struct in_addr client);
  ~VideoStream(void);
  int getError(void) { return errorVal; }

  int sendPacket(custom_IMPEncoderPack *packet);
};

class CommandServer {
private:
  int m_socket_fd = 0;
  int m_client_fd = 0;
  int m_error = 0;
  struct sockaddr_in m_servaddr = {};
  struct sockaddr_in m_client = {};
  bool m_running = false;
  pthread_t m_commandThread = {};
  pthread_rwlock_t m_rwlock = {};

  struct ms_cmd_thread_args {
    bool *isRunningPtr = nullptr;
    pthread_rwlock_t *rwlockPtr = nullptr;
    int client_fd;
  };
  struct ms_cmd_thread_args m_cmd_args = {};
  static void *mf_command_funct(void *arg);
  static uint16_t crc16(const uint8_t *data, size_t length);

public:
  CommandServer(void);
  ~CommandServer(void);
  struct in_addr waitUntilConnected(void);
  bool isConnected(void);
  int getError(void) { return m_running; }
};

#endif // __WEBSERVER_HPP__
