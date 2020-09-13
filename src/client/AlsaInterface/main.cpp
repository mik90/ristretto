#include <docopt/docopt.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>

#include "AlsaInterface.hpp"
#include "Utils.hpp"

static constexpr auto Usage =
    R"(AlsaInterface.

    Usage:
          AlsaInterface (capture | playback) --file <filename>
          AlsaInterface (-h | --help)
          AlsaInterface (-v | --version)

    Options:
          -h --help             Show this screen.
          -v --version          Show the version.
)";

int main(int argc, char** argv) {

  auto args = docopt::docopt(Usage, {std::next(argv), std::next(argv, argc)},
                             true,                 // show help if requested
                             "AlsaInterface 0.1"); // version string
  fmt::print("--------\ndocopt output:\n");
  for (auto const& arg : args) {
    std::cout << arg.first << ": " << arg.second << std::endl;
  }
  fmt::print("--------\n");

  const auto capture = args[std::string("capture")];
  const auto playback = args[std::string("playback")];

  const auto filename = args[std::string("<filename>")];

  mik::Utils::createLogger();
  // Use the default config
  mik::AlsaConfig config;
  config.samplingFreq_Hz = 8000;
  mik::AlsaInterface alsa(config);

  if (capture && filename) {
    std::fstream outputStream(filename.asString(), outputStream.trunc | outputStream.out);

    if (!outputStream.is_open()) {
      SPDLOG_ERROR("Could not open {} for creating/writing", filename.asString());
      std::exit(1);
    }
<<<<<<< HEAD
    alsa.captureAudioFixedSizeMs(outputStream, 5000);
  } else if (playback && filename) {
    std::fstream inputStream(filename.asString(), inputStream.in);
=======
    alsa.captureAudioFixedSize(outputStream, 15);
  } else if (playback && inputAudioFile) {
    std::fstream inputStream(inputAudioFile.asString(), inputStream.in);
>>>>>>> master
    if (!inputStream.is_open()) {
      SPDLOG_ERROR("Could not open {} for reading", filename.asString());
      std::exit(1);
    }
<<<<<<< HEAD
    alsa.playbackAudioFixedSizeMs(inputStream, 5000);
=======
    alsa.playbackAudioFixedSize(inputStream, 15);
>>>>>>> master
  }

  return 0;
}
