#include <cstring>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Webserver.hpp"

// #include <stdio.h>
#include <vector>

VideoStream::VideoStream(int port, struct in_addr client) {
  udpPort = port;
  clientAddr = client;
  memset(&header, 0, sizeof(header));

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    errorVal = errno;
    return;
  }

  header.vpxcc = 0x80;           // version 2
  header.mpt = 96;               // payload type 96 - dynamic h264
  header.ssrc = htonl(0xD00D00); // doodoo lol
}

VideoStream::~VideoStream(void) { close(socket_fd); }

int VideoStream::sendPacket(custom_IMPEncoderPack *packet) {
  if (errorVal != 0) {
    return errorVal;
  }

  constexpr uint8_t delim[] = {0, 0, 0, 1};
  if (0 != memcmp((void *)packet->virAddr, delim, sizeof(delim))) {
    return 1;
  }

  uint32_t nal_length =
      packet->length - 4; // packet, minus Annex B delimiter length
  uint32_t nalPtr = packet->virAddr + 4;

  uint32_t rtp_packet_length = nal_length + sizeof(header);
  uint8_t *ptr = reinterpret_cast<uint8_t *>(packet->virAddr) + 4;

  uint8_t nal_type = ptr[0] & 0x1F;
  // printf("NAL type %u\n", nal_type);

  if (rtp_packet_length <= sizeof(udp_buff)) {
    // packet fits

    if (packet->frameEnd)
      header.mpt |= 0x80;
    else
      header.mpt &= ~0x80;

    memcpy(udp_buff, &header, sizeof(header));
    memcpy(udp_buff + sizeof(header), ptr, nal_length);

    struct sockaddr_in destAddr = {0};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(H264_PORT);
    destAddr.sin_addr = clientAddr;

    ssize_t ret =
        sendto(socket_fd, udp_buff, sizeof(header) + nal_length, 0,
               reinterpret_cast<sockaddr *>(&destAddr), sizeof(destAddr));

    if (ret < 0) {
      errorVal = errno;
      return errorVal;
    }

    m_sequence++;
    header.sequence = htons(m_sequence);

    if (packet->frameEnd) {
      m_timestamp += 3600;
      header.timestamp = htonl(m_timestamp);
    }

    return 0;
  }
  // packet does not fit,
  // uint8_t startPkt = 0b10000000, middlePkt = 0b00000000, endPkt = 0b01000000;

  uint8_t nalHeader = ptr[0];
  uint8_t fuIndecator = (nalHeader & 0xE0) | 28;

  std::vector<uint8_t> payload(ptr + 1, ptr + nal_length);

  constexpr size_t header_crap = sizeof(header) + 2;

  auto it = payload.begin();
  while (it != payload.end()) {
    size_t distance = std::min<size_t>(sizeof(udp_buff) - header_crap,
                                       std::distance(it, payload.end()));

    bool first = (it == payload.begin());
    bool last = (it + distance == payload.end());

    uint8_t fuHeader =
        (first ? 0x80 : 0) | (last ? 0x40 : 0) | (nalHeader & 0x1F);

    if (last && packet->frameEnd) // todo double check this
      header.mpt |= 0x80;
    else
      header.mpt &= ~0x80;

    // memset(udp_buff, 0, sizeof(udp_buff));

    memcpy(udp_buff, &header, sizeof(header));
    udp_buff[sizeof(header)] = fuIndecator;
    udp_buff[sizeof(header) + sizeof(fuIndecator)] = fuHeader;
    memcpy(&udp_buff[header_crap], &(*it), distance);
    // udp_payloads.emplace_back(it, it + distance);

    // sendBuff
    struct sockaddr_in destAddr = {0};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(H264_PORT);
    destAddr.sin_addr = clientAddr;

    ssize_t ret =
        sendto(socket_fd, udp_buff, header_crap + distance, 0,
               reinterpret_cast<sockaddr *>(&destAddr), sizeof(destAddr));

    if (ret < 0) {
      errorVal = errno;
      return errorVal;
    }

    m_sequence++;
    header.sequence = htons(m_sequence);

    it += distance;
  }

  m_timestamp += 3600;
  header.timestamp = htonl(m_timestamp);

  return 0;
}
