#include <memory>

#include <fmt/core.h>

#include "RistrettoClient.hpp"
#include "Utils.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  using namespace mik;
  Utils::createLogger();
  fmt::print("Created logger\n");
  auto channel = grpc::CreateChannel("0.0.0.0:5050", grpc::InsecureChannelCredentials());
  fmt::print("Created channel\n");
  RistrettoClient client(channel);
  fmt::print("Created client\n");

  const auto audioData = Utils::readInAudioFile("./test/resources/ClientTestAudio8KHz.raw");
  const auto transcript = client.decodeAudio(audioData);

  const auto info = fmt::format("Transcript:{}\n", transcript);
  SPDLOG_INFO(info);
  fmt::print(info);
  return 0;
}