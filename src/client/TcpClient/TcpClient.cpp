#include <boost/asio.hpp>
#include <fmt/core.h>
#include <fmt/locale.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "TcpClient.hpp"
#include "Utils.hpp"

namespace mik {

TcpClient::TcpClient() : ioContext_(), socket_(ioContext_) {
  SPDLOG_INFO("--------------------------------------------");
  SPDLOG_INFO("TcpClient created.");
  SPDLOG_INFO("--------------------------------------------");
}

void TcpClient::connect(std::string_view host, std::string_view port) {

  boost::asio::ip::tcp::resolver res(ioContext_);
  boost::asio::ip::tcp::resolver::results_type endpoints;

  try {
    endpoints = res.resolve(host, port);

  } catch (const boost::system::system_error& e) {
    SPDLOG_ERROR("Error {}", e.what());
    throw(e);
  }

  SPDLOG_INFO("Retrieved endpoints");
  SPDLOG_INFO("Attempting to connect to {}:{}...", host, port);

  try {
    boost::asio::connect(socket_, endpoints);
    SPDLOG_INFO("Connected.");

  } catch (const boost::system::system_error& e) {
    SPDLOG_ERROR("Caught exception on connect(): {}", e.what());
    throw(e);
  }
}

size_t TcpClient::sendAudioToServer(const std::vector<char>& buffer) {
  SPDLOG_DEBUG("Writing to the socket...");

  try {
    const boost::asio::const_buffer boostBuffer(buffer.data(), buffer.size());
    SPDLOG_DEBUG(fmt::format(std::locale("en_US.UTF-8"), "Boost buffer is {:L} bytes large",
                             boostBuffer.size()));

    const auto bytesWritten = boost::asio::write(socket_, boostBuffer);
    SPDLOG_DEBUG(fmt::format(std::locale("en_US.UTF-8"), "Wrote {:L} bytes of audio to the socket",
                             bytesWritten));

    return bytesWritten;
  } catch (const boost::system::system_error& e) {
    SPDLOG_ERROR("Couldn't write to socket: {}", e.what());
    return 0;
  }
}

std::string TcpClient::getResultFromServer() {
  // Read in reply
  boost::asio::streambuf streamBuf;
  boost::system::error_code error;
  size_t bytesRead = 0;

  while (!error) {
    bytesRead += boost::asio::read(socket_, streamBuf, error);
  }
  if (error == boost::asio::error::eof) {
    SPDLOG_DEBUG("Read until EOF");
  } else if (error) {
    SPDLOG_ERROR("Got exception while reading from socket: {}", error.message());
    return std::string();
  }

  SPDLOG_DEBUG("Read in {} bytes of results", bytesRead);

  const auto bufIter = streamBuf.data();
  // Create the string based off of the streamBuf range
  const std::string result(boost::asio::buffers_begin(bufIter),
                           boost::asio::buffers_begin(bufIter) + static_cast<long>(bytesRead));
  if (result.empty()) {
    SPDLOG_ERROR("The result from the decoder was empty");
  } else {
    SPDLOG_DEBUG("Result:\"{}\"", result);
  }
  return result;
}

// Strip all the newlines until we get to text
// use all that text until we get to the newline
std::string TcpClient::filterResult(const std::string& fullResult) {
  SPDLOG_INFO("Initial:{}", fullResult);
  // Strip the temporary transcript to get the final result
  auto endOfText = fullResult.crbegin();
  while (*endOfText == '\n' || *endOfText == ' ') {
    // Keep on skipping while the char is a newline or whitespace
    ++endOfText;
  }

  auto startOfText = endOfText;
  while (*startOfText != '\n') {
    // Keep on skipping until we hit another newline
    ++startOfText;
  }

  // Use these as forward iterators
  const auto textSize = endOfText.base() - startOfText.base();
  if (textSize <= 0) {
    SPDLOG_ERROR("textSize is {} which is not positive!", textSize);
    return std::string{};
  }

  const std::string filteredText(startOfText.base(), endOfText.base());
  SPDLOG_INFO("Filtered:{}", filteredText);
  return filteredText;
}

} // namespace mik
