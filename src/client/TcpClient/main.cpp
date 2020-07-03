#include "AlsaInterface.hpp"
#include "TcpClient.hpp"
#include "Utils.hpp"

#include <fmt/core.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

  // Client program for talking to Kaldi's online2bin/online2-tcp-nnet3-decode-faster server

  mik::Utils::createLogger();

  const auto audioData = mik::Utils::readInAudioFile("./test/resources/ClientTestAudio8KHz.raw");
  // TODO Remove Debug audio data
  SPDLOG_DEBUG("audioData as int8_t");
  for (size_t i = 0; i < audioData.size(); ++i) {
    SPDLOG_DEBUG("audioData[{}]:{}", i, static_cast<int8_t>(audioData[i]));
  }

  mik::TcpClient client;
  // Could also do localhost "127.0.0.1"
  // client.connect("192.168.1.165", "5050");
  try {
    client.connect("127.0.0.1", "5050");
  } catch (const boost::system::system_error& e) {
    fmt::print("boost::system::system_error: {}\n", e.what());
    return 1;
  } catch (const std::exception& e) {
    fmt::print("std::exception: {}\n", e.what());
    return 1;
  }
  client.sendAudioToServer(audioData);

  const auto result = client.getResultFromServer();
  fmt::print("Result:\n\"{}\"", result);

  return 0;
}