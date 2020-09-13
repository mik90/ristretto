#include <memory>

#include <docopt/docopt.h>
#include <fmt/core.h>

#include "RistrettoClient.hpp"
#include "Utils.hpp"

static constexpr auto Usage =
    R"(RistrettoClient - Automatic Speech Recognition client

    Usage: RistrettoClient [--file <audio_file>] [--server <server_addr>] [--timeout <timeout_sec>]

    Options:
          -h, --help     Show this screen.
          -v, --version  Show the version.
          --file <audio_file>  pre-recorded audio file to send
          --timeout <timeout_sec>  how long to record for (in seconds)
          --server <server_addr>  ip and port of server   [default: 0.0.0.0:5050]
)";

/**
 * printDocoptOutput()
 * @brief Only used for debugging docopt
 */
void printDocoptOutput(const std::map<std::string, docopt::value>& args) {
  fmt::print("--------\ndocopt output:\n");
  for (auto const& arg : args) {
    // iostreams is able to figure the types a bit better than fmt
    std::cout << arg.first << ": " << arg.second << std::endl;
  }
  fmt::print("--------\n");
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  mik::Utils::createLogger();
  auto args = docopt::docopt(Usage, {std::next(argv), std::next(argv, argc)},
                             true,             // show help if requested
                             "Ristretto 0.1"); // version string
  const auto serverAddr = args[std::string("--server")].asString();
  fmt::print("Server address: {}\n", serverAddr);
  SPDLOG_INFO("Server address: {}", serverAddr);

  mik::RistrettoClient client(grpc::CreateChannel(serverAddr, grpc::InsecureChannelCredentials()));
  fmt::print("Client started\n");

  try {
    const auto audioFile = args[std::string("--file")];
    if (audioFile) {
      // Take in audio file as input
      fmt::print("Processing audio file {}\n", audioFile.asString());
      const auto audioData = mik::Utils::readInAudioFile(audioFile.asString());
      if (audioData.empty()) {
        SPDLOG_WARN("AudioData was empty!");
        fmt::print("AudioData was empty!\n");
        return 1;
      }

<<<<<<< HEAD
      const auto transcript = client.decodeAudioSync(audioData, 0);
      if (transcript.empty()) {
        fmt::print("Response was empty!\n");
        SPDLOG_ERROR("Response was empty!");
        return 1;
      } else {
        fmt::print("Transcript:{}\n", transcript);
        SPDLOG_INFO("Transcript:{}", transcript);
        return 0;
      }

    } else {
      // Get input from user
      const auto timeout = args[std::string("--timeout")];
      if (timeout) {
        const auto timeoutSec = std::chrono::seconds(timeout.asLong());
        client.setRecordingDuration(timeoutSec);
      }
      fmt::print("Processing microphone input\n");
      client.decodeMicrophoneInput();
    }

  } catch (const std::exception& e) {
    SPDLOG_ERROR("Caught std::exception: {}", e.what());
    fmt::print("Caught std::exception: {}\n exiting...", e.what());
    return 1;
  } catch (...) {
    SPDLOG_ERROR("Caught unknown exception");
    fmt::print("Caught unknown exception\n exiting...");
=======
  // TODO Remove Debug audio data
  SPDLOG_DEBUG("audioData as int8_t (first 20 values)");
  for (size_t i = 0; i < 20; ++i) {
    SPDLOG_DEBUG("audioData[{}]:{}", i, static_cast<int8_t>(audioData[i]));
  }

  const auto transcript = client.decodeAudio(audioData);
  if (transcript.empty()) {
    fmt::print("Response was empty!\n");
    SPDLOG_ERROR("Response was empty!");
>>>>>>> master
    return 1;
  }
  return 0;
}