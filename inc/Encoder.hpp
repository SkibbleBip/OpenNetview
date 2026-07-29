#ifndef __ENCODER_HPP__
#define __ENCODER_HPP__

#include "imp/imp_encoder.h"
#include <cstdint>

struct custom_IMPEncoderPack {
  uint32_t phyAddr;
  uint32_t virAddr;
  uint32_t length;
  int64_t timestamp;
  bool frameEnd;
  IMPEncoderNalType dataType;
  // uint8_t buff[8];
};

enum IMPRefType {
  IMP_Encoder_FSTYPE_IDR = 0,
  IMP_Encoder_FSTYPE_LBASE = 1,
  IMP_Encoder_FSTYPE_SBASE = 2,
  IMP_Encoder_FSTYPE_ENHANCE = 3
};
struct StreamReplyData {
  // reverse-engineered
  custom_IMPEncoderPack *pack;
  uint32_t packCount;
  uint32_t seq;
  union {
    IMPRefType refType;
  };
};
/// @todo have to implement my own version of the IMPEncoderStream
/// because OpenIMP's headers dont match that of the original docs

struct StreamReply {
  int error;
  StreamReplyData streamData;
};

class Encoder {
private:
  constexpr static uint32_t width = 1920;
  constexpr static uint32_t height = 1080;
  constexpr static int encChannel = 0;
  int channel = 0;
  int error = 0;
  IMPCell fsCell = {0};
  IMPCell encCell = {0};

public:
  Encoder(int channel);
  Encoder(void) = delete;
  ~Encoder(void);
  int getError(void) { return error; }
  StreamReply getStream(void);
  int releaseStream(StreamReply *reply);
};

#endif // __ENCODER_HPP__
