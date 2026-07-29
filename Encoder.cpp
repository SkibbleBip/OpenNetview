#include "Encoder.hpp"
#include "Channel_init.hpp"
#include "Frame.hpp"
#include "ISP_init.hpp"
#include "imp/imp_system.h"
#include <cstring>
#include <string>

enum IMPPayloadType { PT_JPEG, PT_H264 };
struct custom_IMPEncoderCropCfg {
  bool enable;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
};
struct custom_IMPEncoderUserDataCfg {
  uint32_t maxUserDataCnt;
  uint32_t maxUserDataSize;
};

struct custom_IMPEncoderAttr {
  IMPPayloadType enType;
  uint32_t bufSize;
  uint32_t profile;
  uint32_t picWidth;
  uint32_t picHeight;
  custom_IMPEncoderCropCfg crop;
  custom_IMPEncoderUserDataCfg userData;
};

struct custom_IMPEncoderAttrH264FixQP {
  uint32_t qp;
};
struct custom_IMPEncoderAttrH264CBR {
  uint32_t maxQp;
  uint32_t minQp;
  uint32_t outBitRate;
  int iBiasLvl;
  uint32_t frmQPStep;
  uint32_t gopQPStep;
  bool adaptiveMode;
  bool gopRelation;
};
struct custom_IMPEncoderAttrH264VBR {
  uint32_t maxQp;
  uint32_t minQp;
  uint32_t staticTime;
  uint32_t maxBitRate;
  int32_t iBiasLvl;
  uint32_t changePos;
  uint32_t qualityLvl;
  uint32_t frmQPStep;
  uint32_t gopQPStep;
  bool gopRelation;
};

struct custom_IMPEncoderAttrH264Smart {
  uint32_t maxQp;
  uint32_t minQp;
  uint32_t staticTime;
  uint32_t maxBitRate;
  int32_t iBiasLvl;
  uint32_t changePos;
  uint32_t qualityLvl;
  uint32_t frmQPStep;
  uint32_t gopQPStep;
  bool gopRelation;
};

struct custom_IMPEncoderAttrRcMode {
  IMPEncoderRcMode rcMode;

  union {
    custom_IMPEncoderAttrH264FixQP attrH264FixQp;
    custom_IMPEncoderAttrH264CBR attrH264Cbr;
    custom_IMPEncoderAttrH264VBR attrH264Vbr;
    custom_IMPEncoderAttrH264Smart attrH264Smart;
  };
};

enum EncFrmUsedMode {
  ENC_FRM_BYPASS = 0,
  ENC_FRM_REUSED = 1,
  ENC_FRM_SKIP = 2
};
struct custom_IMPEncoderAttrFrmUsed {
  bool enable;
  EncFrmUsedMode frmUsedMode;
  uint32_t frmUsedTimes;
};

struct custom_IMPEncoderAttrDemask {
  bool enable;
  bool isAutoMode;
  int demaskCnt;
  int demaskThresd;
};

struct custom_IMPEncoderAttrDenoise {
  bool enable;
  int dnType;
  int dnIQp;
  int dnPQp;
};

enum IMPSkipType {
  IMP_Encoder_STYPE_N1X = 0,
  IMP_Encoder_STYPE_N2X = 1,
  IMP_Encoder_STYPE_N4X = 2,
  IMP_Encoder_STYPE_HN1_FALSE = 3,
  IMP_Encoder_STYPE_HN1_TRUE = 4,
  IMP_Encoder_STYPE_H1M_FALSE = 5,
  IMP_Encoder_STYPE_H1M_TRUE = 6
};
struct custom_IMPEncoderAttrInitHSkip {
  IMPSkipType skipType;
  int m;
  int n;
  int maxSameSceneCnt;
  int bEnableScenecut;
  int bBlackEnhance;
};

struct custom_IMPEncoderFrmRate {
  uint32_t frmRateNum;
  uint32_t frmRateDen;
};

struct custom_IMPEncoderRcAttr {
  custom_IMPEncoderFrmRate outFrmRate;
  uint32_t maxGop;
  custom_IMPEncoderAttrRcMode attrRcMode;
  custom_IMPEncoderAttrFrmUsed attrFrmUsed;
  custom_IMPEncoderAttrDemask attrDemask;
  custom_IMPEncoderAttrDenoise attrDenoise;
  custom_IMPEncoderAttrInitHSkip attrHSkip;
};

struct custom_IMPEncoderCHNAttr {
  custom_IMPEncoderAttr encAttr;
  custom_IMPEncoderRcAttr rcAttr;
};
/// @todo discrepancies between openimp and the original libimp
/// I have to recreate the original encoder attributes

Encoder::Encoder(int channelID) {
  error = 0;
  channel = channelID;

  int ret = IMP_System_Init();

  if (ret < 0) {
    std::cerr << "IMP_System_Init failed: " << ret << std::endl;
    error = ret;
    return;
  }

  std::cout << "OpenNetview initialised" << std::endl;

  ret = ISP_init();
  if (ret != 0) {
    error = ret;
    return;
  }

  ret = Channel_init(channel, width, height);
  if (ret != 0) {
    error = ret;
    return;
  }

  ret = IMP_Encoder_CreateGroup(0);
  if (ret != 0) {
    error = ret;
    std::cout << "CreateGroup failed: " << ret << std::endl;
    return;
  }

  custom_IMPEncoderCHNAttr encoderAttr = {};
  encoderAttr.encAttr.enType = PT_H264;
  encoderAttr.encAttr.picWidth = width;
  encoderAttr.encAttr.picHeight = height;
  encoderAttr.encAttr.profile = 1;

  encoderAttr.rcAttr.outFrmRate.frmRateNum = 25;
  encoderAttr.rcAttr.outFrmRate.frmRateDen = 1;

  encoderAttr.rcAttr.maxGop = 25;

  encoderAttr.rcAttr.attrRcMode.rcMode = IMP_ENC_RC_MODE_CBR;

  encoderAttr.rcAttr.attrRcMode.attrH264Cbr.maxQp = 30;
  encoderAttr.rcAttr.attrRcMode.attrH264Cbr.minQp = 10;
  encoderAttr.rcAttr.attrRcMode.attrH264Cbr.outBitRate = 12000; // kbps
  encoderAttr.rcAttr.attrRcMode.attrH264Cbr.frmQPStep = 2;
  encoderAttr.rcAttr.attrRcMode.attrH264Cbr.gopQPStep = 15;

  // hack to get the proprietary imp lib to accept my customised
  // encoder attributes
  ret = IMP_Encoder_CreateChn(encChannel, (IMPEncoderCHNAttr *)&encoderAttr);
  if (ret != 0) {
    error = ret;
    return;
  }

  ret = IMP_Encoder_RegisterChn(0, encChannel);
  if (ret != 0) {
    error = ret;
    return;
  }

  fsCell.deviceID = DEV_ID_FS;
  fsCell.groupID = channel;
  fsCell.outputID = 0;

  encCell.deviceID = DEV_ID_ENC;
  encCell.groupID = 0;
  encCell.outputID = encChannel;

  ret = IMP_System_Bind(&fsCell, &encCell);
  if (ret != 0) {
    error = ret;
    return;
  }
  ret = IMP_Encoder_StartRecvPic(encChannel);
  if (ret != 0) {
    error = ret;
    return;
  }
}

StreamReply Encoder::getStream(void) {
  int ret = IMP_Encoder_PollingStream(encChannel, 1000);
  if (ret != 0) {
    std::cerr << "Timed out" << std::endl;
    StreamReply data = {0};
    data.error = ret;
    return data;
  }
  StreamReplyData rData = {};

  ret = IMP_Encoder_GetStream(encChannel, (IMPEncoderStream *)&rData, 1);
  if (ret != 0) {
    StreamReply data = {0};
    data.error = ret;
    return data;
  }

  StreamReply data = {0};
  data.streamData = rData;
  return data;
}

int Encoder::releaseStream(StreamReply *reply) {
  if (!reply) {
    return -1;
  }
  if (reply->error >= 0)
    return IMP_Encoder_ReleaseStream(encChannel,
                                     (IMPEncoderStream *)&reply->streamData);
  return -1;
}

Encoder::~Encoder(void) {
  int ret = IMP_Encoder_StopRecvPic(encChannel);
  (void)ret;

  ret = IMP_Encoder_UnRegisterChn(encChannel);
  (void)ret;

  ret = IMP_System_UnBind(&fsCell, &encCell);
  (void)ret;

  ret = IMP_Encoder_DestroyChn(encChannel);
  (void)ret;

  ret = IMP_Encoder_DestroyGroup(0);
  (void)ret;

  ret = Channel_close(channel);
  (void)ret;

  ret = ISP_close();
  (void)ret;

  ret = IMP_System_Exit();
  (void)ret;
}
