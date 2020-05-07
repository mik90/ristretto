#pragma once

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>
#include <vector>

#include "AlsaInterface.hpp"

constexpr long MaxLength = 65535;

namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server
// Given a stream of audio, send it over TCP

class DecoderClient {
public:
  DecoderClient();
  void connect(std::string_view host, std::string_view port);
  size_t sendAudioToServer(const std::vector<uint8_t>& buffer);
  std::string getResultFromServer();

private:
  std::shared_ptr<spdlog::logger> logger_;
  boost::asio::io_context ioContext_;
  boost::asio::ip::tcp::socket socket_;
};

class Utils {
public:
  static std::vector<uint8_t> readInAudiofile(std::string_view filename);
};

} // namespace mik