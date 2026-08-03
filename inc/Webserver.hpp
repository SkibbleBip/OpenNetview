#ifndef __WEBSERVER_HPP__
#define __WEBSERVER_HPP__

#include "Encoder.hpp"

#include <netinet/in.h>

#define H264_PORT 6969

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

#endif // __WEBSERVER_HPP__
