#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>

#include "Utils.hpp"

namespace mik {

std::streampos findSizeOfFileStream(std::istream& str) {
  const auto originalPos = str.tellg();    // Find current pos
  str.seekg(0, str.end);                   // Go back to start
  const std::streampos size = str.tellg(); // Find size
  str.seekg(originalPos, str.beg);         // Reset pos back to where we were
  return size;
}

std::vector<uint8_t> Utils::readInAudioFile(std::string_view filename) {
  std::fstream inputStream(std::string(filename), inputStream.in | std::fstream::binary);
  std::istream_iterator<uint8_t> inputIter(inputStream);

  if (!inputStream.is_open()) {
    SPDLOG_ERROR("Could not open inputfile for reading\n");
    return {};
  }

  const auto inputSize = findSizeOfFileStream(inputStream);
  SPDLOG_DEBUG("{} is {} bytes", filename, inputSize);

  // Read in bufferSize amount of bytes from the audio file into the buffer
  std::vector<uint8_t> audioBuffer;
  audioBuffer.reserve(static_cast<std::vector<uint8_t>::size_type>(inputSize));
  SPDLOG_DEBUG("audioBuffer has {} bytes before reading in file", audioBuffer.size());

  // Read in entire file
  std::copy(inputIter, std::istream_iterator<uint8_t>(), std::back_inserter(audioBuffer));
  // inputStream.read(audioBuffer.data(), inputSize);

  SPDLOG_DEBUG("Read in {} out of {} bytes from audio file", inputStream.gcount(), inputSize);
  SPDLOG_DEBUG("audioBuffer has {} bytes after reading in file", audioBuffer.size());

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
  spdlog::set_level(spdlog::level::debug);
  SPDLOG_DEBUG("Debug-level logging enabled");
}

} // namespace mik