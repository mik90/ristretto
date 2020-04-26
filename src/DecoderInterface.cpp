#include <boost/asio.hpp>
#include <fmt/core.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>

#include "DecoderInterface.hpp"

namespace mik {

DecoderInterface::DecoderInterface() : ioContext_(), socket_(ioContext_) {
  try {
    logger_ = spdlog::basic_logger_mt("DecodeInterfaceLogger", "logs/client.log");
  } catch (const spdlog::spdlog_ex &e) {
    std::cerr << "Log init failed with spdlog_ex: " << e.what() << std::endl;
  }

  logger_->info("--------------------------------------------");
  logger_->info("DecoderInterface created.");
  logger_->info("--------------------------------------------");
  logger_->set_level(spdlog::level::level_enum::debug);
  logger_->debug("Debug-level logging enabled");
}

void DecoderInterface::connect(std::string_view host, std::string_view port) {

  boost::asio::ip::tcp::resolver res(ioContext_);

  boost::asio::ip::tcp::resolver::results_type endpoints;
  try {
    endpoints = res.resolve(host, port);

  } catch (const boost::system::system_error &e) {
    logger_->error("Error {}", e.what());
    return;
  }

  logger_->info("Retrieved endpoints");

  try {
    boost::asio::connect(socket_, endpoints);
    logger_->info("Connected to {}:{}", host, port);

  } catch (const boost::system::system_error &e) {
    logger_->info("Caught exception on connect(): {}", e.what());
    return;
  }
}

size_t DecoderInterface::sendAudio(std::vector<char> &buffer) {
  logger_->debug("Writing to the socket...");
  // TODO This should be a const_buffer, but there doesn't seem to be a vector -> const_buffer constructor
  try {
    const auto bytesWritten = boost::asio::write(socket_, boost::asio::buffer(buffer));
    logger_->debug("Wrote {} bytes of audio to the socket", bytesWritten);
    return bytesWritten;
  } catch (const boost::system::system_error &e) {
    logger_->error("Couldn't write to socket: {}", e.what());
    return 0;
  }
}

std::string DecoderInterface::getResult() {
  // Read in reply
  boost::asio::streambuf streamBuf;
  size_t bytesRead;
  try {
    bytesRead = boost::asio::read_until(socket_, streamBuf, "\n");
  } catch (const boost::system::system_error &e) {
    logger_->error("Got exception while reading from socket: {}", e.what());
    return "";
  }
  logger_->debug("Read in {} bytes of results");
  const auto bufIter = streamBuf.data();
  const std::string result(boost::asio::buffers_begin(bufIter), boost::asio::buffers_begin(bufIter) + static_cast<long>(bytesRead));
  if (result.empty()) {
    logger_->error("The result from the decoder was empty");
  }
  return result;
}

std::streampos findSizeOfFileStream(std::istream &str) {
  const auto originalPos = str.tellg();    // Find current pos
  str.seekg(0, str.end);                   // Go back to start
  const std::streampos size = str.tellg(); // Find size
  str.seekg(originalPos, str.beg);         // Reset pos back to where we were
  return size;
}

std::vector<char> readInAudiofile(std::string_view filename) {

  auto logger = spdlog::get("DecodeInterfaceLogger");

  std::fstream inputStream(std::string(filename), inputStream.in | std::fstream::binary);
  if (!inputStream.is_open()) {
    logger->error("Could not open inputfile for reading\n");
    return {};
  }

  const auto inputSize = findSizeOfFileStream(inputStream);
  logger->debug("{} is {} bytes", filename, inputSize);

  // Read in bufferSize amount of bytes from the audio file into the buffer
  std::vector<char> audioBuffer;
  audioBuffer.reserve(static_cast<std::vector<char>::size_type>(inputSize));

  // Read in entire file
  inputStream.read(audioBuffer.data(), inputSize);

  const auto bytesRead = inputStream.gcount();
  logger->debug("Read in {} out of {} bytes from audio file", bytesRead, inputSize);

  return audioBuffer;
}

} // namespace mik

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {

  mik::DecoderInterface iface;
  iface.connect("127.0.0.1", "5050");

  auto audioData = mik::readInAudiofile("audio.raw");
  if (audioData.empty()) {
    return -1;
  }
  iface.sendAudio(audioData);

  const auto result = iface.getResult();
  if (result.empty()) {
    return -1;
  }
  fmt::print("Result: {}", result);

  return 0;
}