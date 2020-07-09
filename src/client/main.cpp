#include <memory>

#include <docopt/docopt.h>
#include <fmt/core.h>

#include "RistrettoClient.hpp"
#include "Utils.hpp"

static constexpr auto Usage =
    R"(RistrettoClient.

    Usage:
          RistrettoClient
          RistrettoClient [(-f | --file) <audio_file>] 
          RistrettoClient [(-cip | --client-ip) <client_ip>] [(-cp | --client-port) <client_port_num>]
          RistrettoClient [(-sip | --server-ip) <server_ip>] [(-sp | --server-port) <server_port_num>]
          RistrettoClient (-h | --help)
          RistrettoClient (-v | --version)

    Options:
          -h --help             Show this screen.
          -v --version          Show the version.
          -f --file             pre-recorded audio file to send
          -cip --client-ip      ip to bind to        [default: 0.0.0.0]
          -cp  --client-port    port client binds to [default: 5050]
          -sip --server-ip      ip of server         [default: 0.0.0.0]
          -sp  --server-port    port server binds to [default: 5050]
)";

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  mik::Utils::createLogger();
  auto args = docopt::docopt(Usage, {std::next(argv), std::next(argv, argc)},
                             true,             // show help if requested
                             "Ristretto 0.1"); // version string
  const auto portNum = args[std::string("port_num")];
  const auto clientIp = args[std::string("client_ip")];
  const auto clientIpAndPort = clientIp.asString() + ":" + portNum.asString();
  auto channel = grpc::CreateChannel(clientIpAndPort, grpc::InsecureChannelCredentials());

  mik::RistrettoClient client(channel);
  fmt::print("Client started\n");

  const auto audioFile = args[std::string("audio_file")];
  if (audioFile) {
    const auto audioData = mik::Utils::readInAudioFile(audioFile.asString());
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

  return 0;
}