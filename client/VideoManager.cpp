
#include "VideoManager.hpp"
#include <iostream>
#include <sstream>

const std::string local_ip = "0.0.0.0";

VideoManager::VideoManager(uint16_t port) {
  std::stringstream sdp_stream;

  sdp_stream << "v=0\r\n"
             << "o=- 0 0 IN IP4 " << local_ip << "\r\n"
             << "s=OpenNetview\r\n"
             << "c=IN IP4 " << local_ip << "\r\n"
             << "t=0 0\r\n"
             << "m=video " << port << " RTP/AVP 96\r\n"
             << "a=rtpmap:96 H264/90000\r\n"
             << "a=fmtp:96 packetization-mode=1\r\n";

  sdp_settings = sdp_stream.str();

  m_cb = {reinterpret_cast<const uint8_t *>(sdp_settings.data()),
          sdp_settings.size()};
  uint8_t *avio_buffer = static_cast<uint8_t *>(av_malloc(4096));

  if (!avio_buffer) {
    m_error = VIDEO_ERROR::AVIO_BUFF_ERROR;
    std::cerr << "Failed to allocate avio buffer" << std::endl;
    return;
  }

  avio_ctx = avio_alloc_context(avio_buffer, 4096, 0, &m_cb, read_packet,
                                nullptr, nullptr);
  if (!avio_ctx) {
    m_error = VIDEO_ERROR::AVIO_ALLOC_CONTEXT_ERROR;
    std::cerr << "Failed to allocate AVIOContext." << std::endl;
    return;
  }

  fmt_ctx = avformat_alloc_context();
  if (!fmt_ctx) {
    m_error = VIDEO_ERROR::AVFORMAT_ALLOC_CONTEXT_ERROR;
    std::cerr << "Failed to allocate AVFormatContext." << std::endl;
    return;
  }
  fmt_ctx->pb = avio_ctx;

  const AVInputFormat *sdp_fmt = av_find_input_format("sdp");
  AVDictionary *options = nullptr;
  av_dict_set(&options, "protocol_whitelist", "file,udp,rtp", 0);

  av_dict_set(&options, "reorder_queue_size", "256", 0);
  av_dict_set(&options, "max_delay", "500000", 0);

  if (avformat_open_input(&fmt_ctx, "memory_sdp", sdp_fmt, &options) < 0) {
    m_error = VIDEO_ERROR::AVFORMAT_OPEN_ERROR;
    av_dict_free(&options);
    std::cerr << "Failed to open in-memory SDP input." << std::endl;
  }

  av_dict_free(&options);

  // discover video stream
  if (avformat_find_stream_info(fmt_ctx, nullptr)) {
    m_error = VIDEO_ERROR::AVFORMAT_FIND_STREAM_ERROR;
    std::cerr << "Failed to find stream info" << std::endl;
    return;
  }

  const AVCodec *decoder = nullptr;

  video_stream_index =
      av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

  if (video_stream_index < 0 || !decoder) {
    m_error = VIDEO_ERROR::AV_FIND_BEST_ERROR;
    std::cerr << "Failed to find best stream" << std::endl;
    return;
  }

  codec_ctx = avcodec_alloc_context3(decoder);

  if (!codec_ctx) {
    m_error = VIDEO_ERROR::AVCODEC_ALLOC_ERROR;
    std::cerr << "Failed to allocated codec" << std::endl;
    return;
  }

  avcodec_parameters_to_context(codec_ctx,
                                fmt_ctx->streams[video_stream_index]->codecpar);

  if (avcodec_open2(codec_ctx, decoder, nullptr) < 0) {
    m_error = VIDEO_ERROR::AVCODEC_OPEN_ERROR;
    std::cerr << "Failed to open decoder" << std::endl;
    return;
  }

  if (!(packet = av_packet_alloc())) {
    m_error = VIDEO_ERROR::AVPACKET_ALLOC_ERROR;
    std::cerr << "Failed to allocate packet" << std::endl;
    return;
  }
  if (!(frame = av_frame_alloc())) {
    m_error = VIDEO_ERROR::AVFRAME_ALLOC_ERROR;
    std::cerr << "Failed to allocate frame" << std::endl;
    return;
  }

  std::cout << "FFmpeg initialized successfully." << std::endl;
}

VideoManager::~VideoManager(void) {

  av_frame_free(&frame);
  av_packet_free(&packet);

  avcodec_free_context(&codec_ctx);

  if (fmt_ctx)
    avformat_close_input(&fmt_ctx);

  if (avio_ctx)
    avio_context_free(&avio_ctx);
}

AVFrame *VideoManager::fetchFrame(void) {

  if (!fmt_ctx || !codec_ctx || !packet || !frame)
    return nullptr;

  while (true) {

    int ret = avcodec_receive_frame(codec_ctx, frame);

    if (ret == 0)
      return frame;

    if (ret != AVERROR(EAGAIN)) {
      std::cerr << "Decoder error: " << ret << std::endl;
      return nullptr;
    }

    // Decoder needs more compressed input
    while (true) {

      ret = av_read_frame(fmt_ctx, packet);

      if (ret < 0) {
        std::cerr << "Demuxer error: " << ret << std::endl;
        return nullptr;
      }

      if (packet->stream_index != video_stream_index) {
        av_packet_unref(packet);
        continue;
      }

      ret = avcodec_send_packet(codec_ctx, packet);

      av_packet_unref(packet);

      if (ret == 0)
        break;

      if (ret != AVERROR(EAGAIN)) {
        std::cerr << "Failed to send packet: " << ret << std::endl;
        return nullptr;
      }

      // Decoder needs output drained before accepting input.
      // Return to avcodec_receive_frame().
      break;
    }
  }
}
