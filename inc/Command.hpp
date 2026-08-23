#ifndef __COMMAND_HPP__
#define __COMMAND_HPP__

#include <cstdint>

constexpr uint8_t START_COMMAND = 0x02;
constexpr size_t MAX_PAYLOAD_SIZE = 255;
// payload consists of 253 bytes followed by a 2 byte crc
// command format:
// | startByte | length of payload | payload[len] | crc uint16_t |

struct commandHeader {
  uint8_t startByte;
  uint8_t len;
};

#endif
