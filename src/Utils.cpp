#include <fmt/locale.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>

#include "Utils.hpp"

namespace mik {

size_t findSizeOfFileStream(std::istream& str) {
  const auto originalPos = str.tellg();    // Find current pos
  str.seekg(0, str.end);                   // Go back to start
  const std::streampos size = str.tellg(); // Find size
  str.seekg(originalPos, str.beg);         // Reset pos back to where we were
  return static_cast<size_t>(size);
}

std::vector<char> Utils::readInAudioFile(std::string_view filename) {
  std::ifstream inputStream(std::string(filename), inputStream.in | std::fstream::binary);

  if (!inputStream.is_open()) {
    SPDLOG_ERROR("Could not open inputfile for reading\n");
    return {};
  }

  const auto inputSize = findSizeOfFileStream(inputStream);
  SPDLOG_DEBUG(fmt::format(std::locale("en_US.UTF-8"), "{} is {:L} bytes", filename, inputSize));

  // Read in the entire file
  std::vector<char> audioBuffer;
  audioBuffer.reserve(inputSize);
  audioBuffer.assign((std::istreambuf_iterator<char>(inputStream)),
                     std::istreambuf_iterator<char>());

  SPDLOG_DEBUG(fmt::format(std::locale("en_US.UTF-8"),
                           "Read in {:L} out of {:L} bytes from audio file", inputStream.gcount(),
                           inputSize));
  SPDLOG_DEBUG(fmt::format(std::locale("en_US.UTF-8"),
                           "audioBuffer has {:L} bytes after reading in file", audioBuffer.size()));
  return audioBuffer;
}

void Utils::createLogger() {
  static std::atomic<bool> hasBeenCalled = false;
  if (hasBeenCalled) {
    return;
  } else {
    hasBeenCalled = true;
  }

  spdlog::set_pattern("[%H:%M:%S] [tid %t] [%s:%#] %v");
  auto logger = spdlog::basic_logger_mt("KaldiClientLogger", "logs/client-kaldi.log", true);
  spdlog::set_default_logger(logger);
  spdlog::flush_every(std::chrono::seconds(3));
  spdlog::set_level(spdlog::level::debug);
  SPDLOG_DEBUG("Debug-level logging enabled");
}

unsigned int Utils::microsecondsToSeconds(unsigned int microseconds) {
  return static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::microseconds(microseconds))
          .count());
}

unsigned int Utils::secondsToMicroseconds(unsigned int seconds) {
  return static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(seconds)).count());
}

} // namespace mik