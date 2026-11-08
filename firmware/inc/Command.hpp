#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

#include <cstdint>

constexpr uint8_t START_COMMAND = 0x02;
constexpr size_t MAX_PAYLOAD_SIZE = 16;


struct Command {
  uint8_t startByte;
  uint8_t payload_bytes[MAX_PAYLOAD_SIZE];
  uint16_t crc;
} __attribute__((packed));



static uint16_t crc16(const uint8_t *data, size_t length) {
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


#endif
