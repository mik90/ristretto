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
  if (audioData.empty()) {
    SPDLOG_WARN("AudioData was empty!");
    fmt::print("AudioData was empty!\n");
    return 1;
  }

  const auto transcript = client.decodeAudio(audioData);
  if (transcript.empty()) {
    fmt::print("Response was empty!\n");
    SPDLOG_ERROR("Response was empty!");
    return 1;
  } else {
    fmt::print("Transcript:{}\n", transcript);
    SPDLOG_INFO("Transcript:{}", transcript);
    return 0;
  }
}