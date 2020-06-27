#pragma once

#include <boost/asio.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "AlsaInterface.hpp"

constexpr long MaxLength = 65535;

namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server
// Given a stream of audio, send it over TCP

std::string filterResult(const std::string& fullResult);

class TcpClient {
public:
  TcpClient();
  void connect(std::string_view host, std::string_view port);
  size_t sendAudioToServer(const std::vector<char>& buffer);
  std::string getResultFromServer();
  static std::string filterResult(const std::string& result);

private:
  boost::asio::io_context ioContext_;
  boost::asio::ip::tcp::socket socket_;
};

} // namespace mik