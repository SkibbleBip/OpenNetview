

#ifndef __VIDEO_MANAGER_HPP__
#define __VIDEO_MANAGER_HPP__

#include <atomic>
#include <cstdint>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

class VideoManager {
private:
  struct CustomBuff {
    const uint8_t *ptr;
    size_t size;
  };

  enum VIDEO_ERROR : int {
    NONE = 0,
    AVIO_BUFF_ERROR = 1,
    AVIO_ALLOC_CONTEXT_ERROR = 2,
    AVFORMAT_ALLOC_CONTEXT_ERROR = 3,
    AVFORMAT_OPEN_ERROR = 4,
    AVFORMAT_FIND_STREAM_ERROR = 5,
    AV_FIND_BEST_ERROR = 6,
    AVCODEC_ALLOC_ERROR = 7,
    AVCODEC_OPEN_ERROR = 8,
    AVPACKET_ALLOC_ERROR = 9,
    AVFRAME_ALLOC_ERROR = 10
  };

  std::string sdp_settings;
  AVIOContext *avio_ctx = nullptr;
  AVFormatContext *fmt_ctx = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  AVPacket *packet = nullptr;
  AVFrame *frame = nullptr;
  std::atomic<bool> *m_isRunning = nullptr;

  CustomBuff m_cb = {0};
  int video_stream_index = 0;
  int m_error = 0;

  static int read_packet(void *ptr, uint8_t *buff, int buff_size);
  static int interruptCallback(void *opaque);

public:
  VideoManager(uint16_t port, std::atomic<bool> *isRunning);
  ~VideoManager(void);
  const int getError(void) const { return m_error; }
  AVFrame *fetchFrame(void);
  void abort(void);
};

#endif // __VIDEO_MANAGER_HPP__
