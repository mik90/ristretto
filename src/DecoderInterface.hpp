#pragma once

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>
#include <vector>

constexpr long MaxLength = 65535;

namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server

// Given a stream of audio, send it over TCP

// aka snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
using AudioType = char;

class DecoderInterface {
public:
  DecoderInterface();
  void connect(std::string_view host, std::string_view port);
  size_t sendAudio(std::vector<AudioType> &buffer);
  std::string getResult();

private:
  std::shared_ptr<spdlog::logger> logger_;
  boost::asio::io_context ioContext_;
  boost::asio::ip::tcp::socket socket_;
};

} // namespace mik