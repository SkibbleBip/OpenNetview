#include <cstring>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Webserver.hpp"

// #include <stdio.h>
#include <iostream>
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

#include <pthread.h>

CommandServer::CommandServer(void) : m_running(false), m_commandThread(0) {
  m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket_fd < 0) {
    m_error = errno;
    return;
  }
  m_servaddr.sin_family = AF_INET;
  m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  m_servaddr.sin_port = htons(COMMAND_PORT);
  if (0 !=
      bind(m_socket_fd, (struct sockaddr *)&m_servaddr, sizeof(m_servaddr))) {
    m_error = errno;
    return;
  }

  if (0 != listen(m_socket_fd, 1)) {
    m_error = errno;
    return;
  }

  m_rwlock = PTHREAD_RWLOCK_INITIALIZER;
}

CommandServer::~CommandServer(void) {
  if (m_client_fd >= 0) {
    close(m_client_fd);
  }

  pthread_join(m_commandThread, nullptr);

  pthread_rwlock_destroy(&m_rwlock);

  close(m_socket_fd);
}

uint16_t CommandServer::crc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;

    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

void *CommandServer::mf_command_funct(void *arg) {
  struct ms_cmd_thread_args *cmd_args =
      static_cast<struct ms_cmd_thread_args *>(arg);
  bool *isRunning = cmd_args->isRunningPtr;
  pthread_rwlock_t *lock = cmd_args->rwlockPtr;
  int client_fd = cmd_args->client_fd;

  bool isError = false;

  int ret = 0;
  while (true) {
    // do stuff to handle commands
    struct commandHeader header = {0};
    ret = recv(client_fd, &header.startByte, sizeof(uint8_t), 0);
    if (ret < 0) {
      std::cerr << "Failed to recv() from client (start byte): " << errno
                << std::endl;
      // return (void*)(intptr_t)errno;
      isError = true;
      break;
    } else if (ret == 0) {
      // connection terminated
      break;
    }
    if (header.startByte != START_COMMAND) {
      continue;
      // invalid packet byte,
    }

    ret = recv(client_fd, &header.len, sizeof(uint8_t), 0);
    if (ret < 0) {
      std::cerr << "Failed to recv() from client (len byte): " << errno
                << std::endl;
      // return (void*)(intptr_t)errno;
      isError = true;
      break;
    } else if (ret == 0) {
      // connection terminated
      break;
    } else {
      struct pollfd cmdSck = {0};

      cmdSck.fd = client_fd;
      cmdSck.events = POLLIN;
      cmdSck.revents = 0;

      uint8_t recvBytes = 0;
      uint8_t payload[MAX_PAYLOAD_SIZE] = {0};
      bool timedOutOrClosed = false;
      do {

        ret = poll(&cmdSck, 1, 1000);
        //  1 sec timeout

        if (ret == 0) {
          // timeout
          timedOutOrClosed = true;
          break;
        } else if (ret < 0) {
          // error
          std::cerr << "Failed to poll() from client for payload: " << errno
                    << std::endl;
          isError = true;
          break;
        }

        ret = recv(client_fd, &payload[recvBytes], header.len - recvBytes, 0);
        if (ret < 0) {
          std::cerr << "Failed to read command payload: " << errno << std::endl;
          isError = true;
          break;
        }
        if (ret == 0) {
          // connection terminated
          timedOutOrClosed = true;
          break;
        }

        recvBytes += ret;

      } while (recvBytes < header.len);

      if (isError == true)
        break;
      else if (timedOutOrClosed == true)
        continue;

      uint16_t crc_to_test = (uint16_t)(payload[header.len - 2] << 8) |
                             (uint16_t)(payload[header.len - 1]);
      crc_to_test = htons(crc_to_test);

      uint16_t crc = crc16(payload, header.len - 2);

      if (crc_to_test != crc) {
        std::cerr << "Warning: CRC missmatch: " << crc_to_test << " vs " << crc
                  << std::endl;
      } else {
        /// todo: handle commands
      }

    } // end else

  } // while true

  ret = pthread_rwlock_wrlock(lock);
  if (ret < 0) {
    isError = true;
  } else {
    *isRunning = false;
    ret = pthread_rwlock_unlock(lock);
    if (ret < 0) {
      isError = true;
    }
  }

  std::cout << "Closing connection" << std::endl;
  return (isError ? (void *)(intptr_t)errno : nullptr);
}

struct in_addr CommandServer::waitUntilConnected(void) {
  size_t len = sizeof(m_client);
  m_client_fd = accept(m_socket_fd, (struct sockaddr *)&m_client, &len);
  if (m_client_fd < 0) {
    m_error = errno;
    std::cerr << "Failed to wait for client connection: " << m_error
              << std::endl;
    return {0};
  }

  struct in_addr ip = m_client.sin_addr;
  m_running = true;

  m_cmd_args = {&m_running, &m_rwlock, m_client_fd};

  int ret =
      pthread_create(&m_commandThread, nullptr, mf_command_funct, &m_cmd_args);
  if (ret < 0) {
    m_error = errno;
    std::cerr << "Failed to create command thread: " << m_error << std::endl;
    return {0};
  }

  return ip;
}

bool CommandServer::isConnected(void) {
  int ret = pthread_rwlock_rdlock(&m_rwlock);
  if (ret < 0) {
    m_error = errno;
    return false;
  }
  bool cacheRunning = m_running;
  ret = pthread_rwlock_unlock(&m_rwlock);
  if (ret < 0) {
    m_error = errno;
    return false;
  }
  return cacheRunning;
}
